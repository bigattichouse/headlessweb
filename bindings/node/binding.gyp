{
  "targets": [
    {
      "target_name": "hweb_addon",
      "sources": [
        "hweb_addon.cpp",
        "browser_wrapper.cpp",
        "session_wrapper.cpp",
        "utils.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../src",
        "../../src/Browser",
        "../../src/Session",
        "../../src/FileOps",
        "../../src/Assertion"
      ],
      "libraries": [
        "-L../../build",
        "-pthread"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "cflags": [
        "-std=c++17",
        "-fPIC"
      ],
      "cflags_cc": [
        "-std=c++17",
        "-fPIC"
      ],
      "defines": [ 
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "NAPI_VERSION=6"
      ],
      "conditions": [
        ["OS==\"linux\"", {
          "libraries": [
            "<!@(pkg-config --libs gtk4)",
            "<!@(pkg-config --libs webkit2gtk-6.0)",
            "<!@(pkg-config --libs jsoncpp)",
            "<!@(pkg-config --libs cairo)",
            "<!@(pkg-config --libs gdk-pixbuf-2.0)"
          ],
          "cflags": [
            "<!@(pkg-config --cflags gtk4)",
            "<!@(pkg-config --cflags webkit2gtk-6.0)",
            "<!@(pkg-config --cflags jsoncpp)",
            "<!@(pkg-config --cflags cairo)",
            "<!@(pkg-config --cflags gdk-pixbuf-2.0)"
          ]
        }],
        ["OS==\"mac\"", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.15"
          }
        }]
      ]
    }
  ]
}