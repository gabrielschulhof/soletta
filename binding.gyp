{
	"conditions": [
		[ "'<!(echo $SOLETTA_FROM_MAKE)'!='true'", {
			"targets+": [
				{
					"target_name": "csdk",
					"type": "none",
					"actions": [ {
						"action_name": "build-csdk",
						"message": "Building C SDK",
						"outputs": [ "build/soletta_sysroot" ],
						"inputs": [ "" ],
						"action": [ "sh", "-c",
							"unset PYTHON && unset PYTHON_PATH && make alldefconfig && make -j9"
						]
					} ]
				}
			]
		} ]
	],
	"targets": [
		{
			"target_name": "collectbindings",
			"type": "none",
			"actions": [ {
				"action_name": "collectbindings",
				"message": "Collecting bindings",
				"outputs": [ "bindings/nodejs/generated/main.cc" ],
				"inputs": [
					"bindings/nodejs/generated/main.cc.prologue",
					"bindings/nodejs/generated/main.cc.epilogue",
				],
				"action": [ "sh", "-c", "cd ./bindings/nodejs && ./generate-main.sh" ]
			} ],

			# Ensure that soletta is built first if the build process starts with npm install
			"conditions": [
				[ "'<!(echo $SOLETTA_FROM_MAKE)'!='true'", {
					"dependencies": [ "csdk" ]
				} ]
			]
		},
		{
			"target_name": "soletta",
			"sources": [
				"bindings/nodejs/generated/main.cc",
				"bindings/nodejs/src/functions/simple.cc"
			],
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
				"build/soletta_sysroot/usr/include/soletta"
			],
			"libraries": [
				'<!@(echo "-L$(pwd)/build/soletta_sysroot/usr/lib")',
				"-lsoletta"
			],
			"dependencies": [ "collectbindings" ]
		},
		{
			"target_name": "dlfcn",
			"sources": [ "bindings/nodejs/dlfcn/dlfcn.cc" ],
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
			]
		}
	]
}
