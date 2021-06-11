add_rules("mode.debug", "mode.release")

add_requires("zstd", {debug = is_mode("debug")})
target("DgaImage")
    add_rules("qt.static")
    add_packages("zstd")
    add_frameworks("core", "QtGui")
    add_defines("QT_STATICPLUGIN")
    add_includedirs("../include")
    add_includedirs("./")
    add_files("*.cpp") 
    add_files("Dgaimageplugin.h")
    