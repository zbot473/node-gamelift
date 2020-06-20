{
    'targets': [
        {
            'target_name': 'node-gamelift',
            'sources': ['src/node_gamelift.cpp'],
            'include_dirs': [
                "<!@(node -p \"require('node-addon-api').include\")",
                "<!@(node -p \"require('napi-thread-safe-callback').include\")",
                "gamelift/include"
            ],
            'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
            'cflags!': ['-fno-exceptions'],
            'cflags_cc!': ['-fno-exceptions'],
            'xcode_settings': {
                'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                'CLANG_CXX_LIBRARY': 'libc++',
                'MACOSX_DEPLOYMENT_TARGET': '10.7'
            },
            'msvs_settings': {
                'VCCLCompilerTool': {'ExceptionHandling': 1},
            },
            'conditions': [
                ['OS == "mac"', {
                    'libraries': [
                        '-L../gamelift/lib/darwin-x64',
                        '-laws-cpp-sdk-gamelift-server',
                        '-Wl,-rpath,@loader_path/../../gamelift/lib/darwin-x64'
                    ]
                }],
                ['OS == "linux"', {
                    'libraries': [
                        '-L../gamelift/lib/linux-x64',
                        '-laws-cpp-sdk-gamelift-server',
                        '-Wl,-rpath,\$$ORIGIN/../../gamelift/lib/linux-x64'
                    ]
                }]
            ]
        }
    ]
}
