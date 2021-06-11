#include "OcTree.h"
#include <QSet>
#if defined(__GNUC__)
#define omp_get_num_procs()     (1)
#else
#include <omp.h>
#endif
#include <QDebug>

OcTree::OcTree(int maxcolors)
{
    this->maxcolors = maxcolors;
    this->trunk = new ONode;
    memset(this->trunk, 0, sizeof(ONode));
    INIT_LIST_HEAD(&this->trunk->cache_list);
    this->spares = NULL;
}

int OcTree::BuildOctree(const QImage& img, const QRect bbox, uint8_t* transMask)
{
    this->palette_valid = false;
    int y;
    const int bytesPrePix = img.depth() / 8;
    const int h = img.height();
    const int w = img.width();
    const int rowspan = img.bytesPerLine();
    const uint8_t* bits = img.constBits();
    memset(mCache, 0, sizeof(mCache));
    memset(mPaletteCache, 0xff, sizeof(mPaletteCache));
    register QRgb pxVal;
    for (y = bbox.y(); y <= bbox.bottom(); ++y)
    {
        const QRgb* px = (const QRgb*)(bits + y * rowspan + bbox.x() * bytesPrePix);
        const QRgb* pxEnd = (const QRgb*)(bits + y * rowspan + bbox.right() * bytesPrePix);
        const uint8_t* pxMask = (transMask + y * w + bbox.x());
        while (px <= pxEnd)
        {
            if (!transMask || *(pxMask++)) {
                pxVal = (*px);
                AddColorToTree(pxVal);
                if (this->leafcount > this->maxcolors) PruneTree();
            }
            px = (const QRgb*)((uint8_t *)px + bytesPrePix);
        }
    }
    return this->leafcount;
}
void OcTree::DumpIndexData(uint8_t* indexData, const QImage& img,
    const QRect bbox, uint8_t* transMask, uint8_t transIndex) 
{
    if (!this->palette_valid) CollectLeaves();
    _DumpIndexData(indexData, img, bbox, transMask, transIndex);
    return;
}


void OcTree::_DumpIndexData(uint8_t* indexData, const QImage& img, 
    const QRect bbox, uint8_t* transMask, uint8_t transIndex)
{
    int y;
    const int bytesPrePix = img.depth() / 8;
    const int h = img.height();
    const int w = img.width();
    const int rowspan = img.bytesPerLine();
    const uint8_t* bits = img.constBits();
    for (y = bbox.y(); y <= bbox.bottom(); ++y)
    {
        const QRgb* linePx = (const QRgb*)(bits + y * rowspan);
        const QRgb* pxEnd = (const QRgb*)(bits + y * rowspan + bbox.right() * bytesPrePix);
        const uint8_t* pxMask = (transMask + y * w);
        uint8_t* indexPtr = (indexData + y * w);
        memset(indexPtr, transIndex, w);
        int x;
        for(x = bbox.x();x <= bbox.right(); x++)
        {
            const QRgb* px = (const QRgb*)((uint8_t*)linePx + bytesPrePix * x);
            if (!transMask || pxMask[x]) {
                QRgb inxRgb = (*px) & CACHE_MASK;
                int cache_idx = (inxRgb) % OcTreeCacheSize;
                if(mPaletteCache[cache_idx].inxVal == inxRgb
                  && mPaletteCache[cache_idx].pattleInx != 0xff) {
                    mPaletteCache_hits++;
                    indexPtr[x] = mPaletteCache[cache_idx].pattleInx;
                } else{
                    indexPtr[x] = FindColorInTree(*px);
                    mPaletteCache[cache_idx].pattleInx = indexPtr[x];
                    mPaletteCache[cache_idx].inxVal = inxRgb;
                    mPaletteCache_nohits ++;
                }
            }
        }
    }
}

int OcTree::ExtractOctreePalette(RGB* palette)
{
    if (!palette) return 0;

    if (!this->palette_valid) CollectLeaves();

    if (this->palette) memcpy(palette, this->palette, this->maxcolors * sizeof(RGB));

    return this->leafcount;
}
 
int OcTree::FindInOctree(QRgb color)
{
    if (!this->palette_valid) CollectLeaves();
    int idx = FindColorInTree(color);
    return idx;
}

void OcTree::ResetOctree(int maxc)
{
    if (maxc > this->maxcolors)
    {
        free(this->palette);
        this->palette = 0;
    }

    DeleteNode(nullptr, this->trunk, &this->spares);
    this->leafcount = 0;
    this->maxcolors = maxc;
    this->palette_valid = false;
    memset(this->branches, 0, sizeof(this->branches));

    this->trunk = this->spares;
    if (this->trunk) this->spares = this->trunk->next;
    else this->trunk = new ONode;

    memset(this->trunk, 0, sizeof(ONode));
    INIT_LIST_HEAD(&this->trunk->cache_list);
}

OcTree::~OcTree()
{
    DeleteNode(nullptr, this->trunk, NULL);

    ONode* p = this->spares;
    while (p)
    {
        ONode* del = p;
        p = p->next;

        delete del;
    }
    if(this->palette)free(this->palette);
    qDebug() << "hits " << mCache_hits 
        << ", nohits:" << mCache_nohits
        << "hit rate :" << mCache_hits / (float)(mCache_nohits + mCache_hits);
        
    qDebug() << "Pattle cache hits " << mPaletteCache_hits 
        << ", nohits:" << mPaletteCache_nohits
        << "hit rate :" << mPaletteCache_hits / (float)(mPaletteCache_nohits + mPaletteCache_hits);

}

inline int OcTree::FindColorInTreeCache(const QRgb rgb)
{
    QRgb inxRgb = rgb & CACHE_MASK;
    int cache_idx = (inxRgb) % OcTreeCacheSize;
    ONode* np = mCache[cache_idx].node;
    if(!np || mCache[cache_idx].inxVal != inxRgb)return -1;
    return np->leafidx;
}

int OcTree::FindColorInTree(const QRgb rgb)
{
    int i;
    i = FindColorInTreeCache(rgb);
    if(i > -1) return i;
    ONode* p = this->trunk;

    const unsigned char r = qRed(rgb);
    const unsigned char g = qGreen(rgb);
    const unsigned char b = qBlue(rgb);
    for (i = OCTREE_DEPTH - 1; i >= 0; --i)
    {
        if (!p->childflag) break;

        const int j = i + 8 - OCTREE_DEPTH;
        const unsigned char idx = (((r >> (j - 2)) & 4)) | (((g >> (j - 1)) & 2)) | ((b >> j) & 1);

        ONode* np = p->children[idx];
        if (!np) break;

        p = np;
    }
    return p->leafidx;
}

int OcTree::CollectLeaves()
{
    if (!this->palette) this->palette = (RGB*)malloc(this->maxcolors * sizeof(RGB));

    if (!this->palette) return 0;

    int sz = CollectNodeLeaves(this->trunk, this->palette, 0);
    memset(this->palette + sz, 0, (this->maxcolors - sz) * sizeof(RGB));
    this->palette_valid = true;

    return sz;
}
inline bool OcTree::AddColorToTreeCache(const QRgb rgb)
{
    QRgb inxRgb = rgb & CACHE_MASK;
    int cache_idx = ((quint32)inxRgb) % OcTreeCacheSize;
    ONode* np = mCache[cache_idx].node;
    if(!np || mCache[cache_idx].inxVal != inxRgb)return false;
    np->sumrgb[0] += qRed(rgb);
    np->sumrgb[1] += qGreen(rgb);
    np->sumrgb[2] += qBlue(rgb);
    np->colorcount++;
    return true;
}

inline void OcTree::AddNodeToTreeCache(const QRgb rgb, ONode* np)
{
    QRgb inxRgb = rgb & CACHE_MASK;
    int cache_idx = (inxRgb) % OcTreeCacheSize;
    auto cache_node = &mCache[cache_idx];
    if(cache_node->node){
        list_del(&cache_node->listnode);
    }
    cache_node->node = np;
    cache_node->inxVal = inxRgb;
    list_add(&cache_node->listnode, &np->cache_list);
    return;
}

void OcTree::AddColorToTree(const QRgb rgb)
{
    ONode* p = this->trunk;
    p->colorcount++;

    if(this->AddColorToTreeCache(rgb)){
        mCache_hits++;
        return;
    }
    mCache_nohits++;
    int i;
    const unsigned char r = qRed(rgb);
    const unsigned char g = qGreen(rgb);
    const unsigned char b = qBlue(rgb);
    for (i = OCTREE_DEPTH - 1; i >= 0; --i)
    {
        const int j = i + 8 - OCTREE_DEPTH;
        const unsigned char idx = (((r >> (j - 2)) & 4)) | (((g >> (j - 1)) & 2)) | ((b >> j) & 1);

        ONode* np = p->children[idx];
        bool isleaf = false;

        if (np)
        {
            isleaf = !np->childflag;
        }
        else // add node
        {
            if (!p->childflag) // first time down this path
            {
                p->childflag = idx + 1;
            }
            else if (p->childflag > 0)  // creating a new branch
            {
                p->childflag = -1;
                p->next = this->branches[i];
                this->branches[i] = p;
            }
            // else multiple branch, which we don't care about

            np = this->spares;
            if (np) this->spares = np->next;
            else np = new ONode;

            p->children[idx] = np;
            memset(np, 0, sizeof(ONode));
            INIT_LIST_HEAD(&np->cache_list);
        }

        if (isleaf) {
            np->sumrgb[0] += r;
            np->sumrgb[1] += g;
            np->sumrgb[2] += b;
            np->colorcount++;
            AddNodeToTreeCache(rgb, np);
            return;
        }

        p = np; // continue downward
    }

    // p is a new leaf at the bottom
    p->sumrgb[0] += r;
    p->sumrgb[1] += g;
    p->sumrgb[2] += b;
    p->colorcount++;
    AddNodeToTreeCache(rgb, p);
    this->leafcount++;
}

int OcTree::CollectNodeLeaves(ONode* p, RGB* palette, int colorcount)
{
    if (!p->childflag)
    {
        p->leafidx = colorcount;
        palette[colorcount].r = (int)((double)p->sumrgb[0] / (double)p->colorcount);
        palette[colorcount].g = (int)((double)p->sumrgb[1] / (double)p->colorcount);
        palette[colorcount].b = (int)((double)p->sumrgb[2] / (double)p->colorcount);
        colorcount++;
    }
    else
    {
        if (p->childflag > 0)
        {
            colorcount = CollectNodeLeaves(p->children[p->childflag - 1], palette, colorcount);
        }
        else
        {
            int i;
            for (i = 0; i < 8; ++i)
            {
                if (p->children[i])
                {
                    colorcount = CollectNodeLeaves(p->children[i], palette, colorcount);
                }
            }
        }
        // this is a branch or passthrough node,  record the index
        // of any downtree leaf here so that we can return it for
        // color lookups that want to diverge off this node 
        p->leafidx = colorcount - 1;
    }

    // colorcount should == leafcount
    return colorcount;
}

void OcTree::DeleteNode(ONode* parent, ONode* p, ONode** delete_to)
{
    if (!p->childflag)
    {
        this->leafcount--;
    }
    else if (p->childflag > 0)
    {
        DeleteNode(parent, p->children[p->childflag - 1], delete_to);
        p->children[p->childflag - 1] = nullptr;
        p->childflag = 0;
    }
    else
    {
        int i;
        for (i = 0; i < 8; ++i)
        {
            if (p->children[i])
            {
                DeleteNode(parent, p->children[i], delete_to);
                p->children[i] = nullptr;
            }
        }
        p->childflag = 0; // now it's a leaf
    }
    if(parent){
        parent->sumrgb[0] += p->sumrgb[0];
        parent->sumrgb[1] += p->sumrgb[1];
        parent->sumrgb[2] += p->sumrgb[2];
        parent->colorcount+= p->colorcount;
    }
    list_head *pos, *tmp;
    list_for_each_safe(pos, tmp, &p->cache_list){
        ONodeCache* tmpNode = CONTAINER_OF(pos, ONodeCache, listnode);
        tmpNode->node = 0;
        list_del(&tmpNode->listnode);
    }
    if (delete_to)
    {
        p->next = *delete_to;
        *delete_to = p;
    }
    else
    {
        delete p;
    }
}

int OcTree::PruneTree()
{
    ONode* branch = 0;
    int i;
    for (i = 0; i < OCTREE_DEPTH; ++i) // prune at the furthest level from the trunk
    {
        branch = this->branches[i];
        if (branch)
        {
            this->branches[i] = branch->next;
            branch->next = 0;
            break;
        }
    }

    if (branch)
    {
        int i;
        for (i = 0; i < 8; ++i)
        {
            if (branch->children[i])
            {
                DeleteNode(branch, branch->children[i], &this->spares);
                branch->children[i] = 0;
            }
        }
        branch->childflag = 0; // now it's a leaf
        this->leafcount++;
    }

    return this->leafcount;
}
