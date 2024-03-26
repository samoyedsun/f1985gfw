add_rules("mode.release")

add_requires("boost 1.80.0")
add_requires("protobuf-cpp 3.19.4")
add_requires("lua v5.4.4")
add_requires("openssl 1.1.1-t")

add_requires("xlnt")
add_requires("asio")
add_requires("imgui")
add_requires("glfw")
add_requires("glad")
add_requires("openssl")
add_requires("nlohmann_json")
add_requires("mimalloc")
add_requires("gtest")

target("server")
    set_kind("binary")
    set_installdir("server")
    set_languages("cxx20")
    add_rules("protobuf.cpp")
    add_files("server/*.cpp")
    --add_headerfiles("server/*.hpp")
    --add_headerfiles("server/*.h")
    add_headerfiles("source/*.hpp")
    add_files("proto/*.proto")
    add_packages("boost")
    add_packages("protobuf-cpp")
    add_packages("lua")
    add_packages("openssl")

target("client")
    set_kind("binary")
    set_installdir("client")
    set_languages("cxx20")
    add_rules("protobuf.cpp")
    add_files("client/*.cpp")
    --add_headerfiles("client/*.hpp")
    --add_headerfiles("client/*.h")
    add_headerfiles("source/*.hpp")
    add_files("proto/*.proto")
    add_packages("boost")
    add_packages("protobuf-cpp")
    add_packages("lua")
    add_packages("openssl")
    --add_links("ssl", "crypto")

target("tool")
    set_kind("binary")
    set_installdir("publish/tool")
    set_languages("cxx20")
    add_files("tool/*.cpp")
    add_packages("xlnt")
    add_packages("asio")
    add_packages("imgui")
    add_packages("glfw")
    add_packages("glad")
    add_packages("openssl")
    add_packages("nlohmann_json")
    add_packages("mimalloc")
    add_packages("gtest")

target("xlsx2json")
    set_kind("binary")
    set_installdir("publish/xlsx2json")
    set_languages("cxx20")
    add_files("xlsx2json/*.cpp")
    add_packages("xlnt")
    add_packages("asio")
    add_packages("imgui")
    add_packages("glfw")
    add_packages("glad")
    add_packages("openssl")
    add_packages("nlohmann_json")
    add_packages("mimalloc")
    add_packages("gtest")
    add_links("ole32")
    
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

