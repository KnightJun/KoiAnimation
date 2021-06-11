#pragma once
#include <QImage>
#include <ImageUtils.h>
#include <hlist.h>
#define OCTREE_DEPTH 5  // every depth level adds 3 bits of RGB colorspace (depth 8 => 24-bit RGB)
#define CACHE_MASK 0xF8F8F8
struct ONodeCache;
struct ONode
{
	int64_t colorcount = 0;  // number of color instances at or below this node
	int64_t sumrgb[3] = {0};
	int childflag = 0;   // 0=leaf, >0=index of single child, <0=branch
	int leafidx = 0;     // populated at the end
	ONode* next = 0;     // for OTree::branches
	ONode* children[8] = {0};
	list_head cache_list;
};

struct PattleCache
{
	quint8 pattleInx;
	QRgb inxVal;
};

struct ONodeCache
{
	ONode *node;
	QRgb inxVal;
	list_head listnode;
};
#define OcTreeCacheSize 997
class OcTree
{
public:
	OcTree(int maxcolors);
	int BuildOctree(const QImage& img, const QRect bbox, uint8_t *transMask = nullptr);
	void DumpIndexData(uint8_t* indexData, const QImage& img, 
		const QRect bbox, uint8_t* transMask = nullptr, uint8_t transIndex = 0xff);
	int ExtractOctreePalette(struct RGB* palette);
	int FindInOctree(QRgb color);
	void ResetOctree(int maxc);
	~OcTree();
private:
	void _DumpIndexData(uint8_t* indexData, const QImage& img,
		const QRect bbox, uint8_t* transMask = nullptr, uint8_t transIndex = 0xff);
	int FindColorInTreeCache(const QRgb rgb);
	int FindColorInTree(const QRgb rgb);
	int CollectLeaves();
	bool AddColorToTreeCache(const QRgb rgb);
	void AddNodeToTreeCache(const QRgb rgb, ONode* np);
	void AddColorToTree(const QRgb rgb);
	int CollectNodeLeaves(ONode* p, struct RGB* palette, int colorcount);
	void DeleteNode(ONode* parent, ONode* p, ONode** delete_to);
	int PruneTree();
	int maxcolors = 0;
	int leafcount = 0;
	ONode* trunk = 0;
	ONode* branches[OCTREE_DEPTH] = { 0 };  // linked lists of branches for each level of the tree
	ONode* spares = 0;
	struct RGB* palette = 0;  // populated at the end
	bool palette_valid = 0;

	ONodeCache mCache[OcTreeCacheSize];
	quint64 mCache_hits = 0;
	quint64 mCache_nohits = 0;
	PattleCache  mPaletteCache[OcTreeCacheSize];
	quint64 mPaletteCache_hits = 0;
	quint64 mPaletteCache_nohits = 0;
};

