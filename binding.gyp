{
	"variables": {
		"BUILD_SOLETTA": '<!(sh bindings/nodejs/establish-flags.sh BUILD_SOLETTA)',
		"SOLETTA_CFLAGS": [ '<!@(sh bindings/nodejs/establish-flags.sh SOLETTA_CFLAGS)' ],
		"SOLETTA_LIBS": [ '<!@(sh bindings/nodejs/establish-flags.sh SOLETTA_LIBS)' ]
	},
	"conditions": [
		[ "'<(BUILD_SOLETTA)'=='true'", {
			"targets": [
				{
					"target_name": "csdk",
					"type": "none",
					"actions": [ {
						"action_name": "build-csdk",
						"message": "Building C SDK",
						"outputs": [ "build/soletta_sysroot" ],
						"inputs": [ "" ],
						"action": [ "sh", "bindings/nodejs/build-for-npm.sh" ]
					} ]
				}
			]
		}, {
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
						"action": [
							"sh",
							"-c",
							'./bindings/nodejs/generate-main.sh <(SOLETTA_CFLAGS)'
						]
					} ]
				},
				{
					"target_name": "soletta",
					"sources": [
						"bindings/nodejs/generated/main.cc",
						"bindings/nodejs/src/async-bridge.cc",
						"bindings/nodejs/src/data.cc",
						"bindings/nodejs/src/hijack.c",
						"bindings/nodejs/src/functions/network-address.cc",
						"bindings/nodejs/src/functions/oic-client.c",
						"bindings/nodejs/src/functions/oic-client-discovery.cc",
						"bindings/nodejs/src/functions/oic-client-get-platform-info.cc",
						"bindings/nodejs/src/functions/oic-client-observation.cc",
						"bindings/nodejs/src/functions/simple.cc",
						"bindings/nodejs/src/functions/sol-platform-monitors.cc",
						"bindings/nodejs/src/structures/device-id.cc",
						"bindings/nodejs/src/structures/network-link-addr.cc",
						"bindings/nodejs/src/structures/oic-map-reader.cc",
						"bindings/nodejs/src/structures/oic-platform-and-server-info.cc",
						"bindings/nodejs/src/structures/oic-resource.cc"
					],
					"include_dirs": [
						"<!(node -e \"require('nan')\")"
					],
					"cflags": [ '<(SOLETTA_CFLAGS)' ],
					"xcode_settings": {
						"OTHER_CFLAGS": [ '<(SOLETTA_CFLAGS)' ]
					},
					"libraries": [ '<(SOLETTA_LIBS)' ],
					"dependencies": [ "collectbindings" ]
				}
			]
		} ]
	]
}
