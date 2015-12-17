{
	"target_defaults": {
		"include_dirs": [
			"<!(node -e \"require('nan')\")",

			# Needed for generated files
			"<!@(echo \"$(pwd)/src\")"
		]
	},

	"targets": [
		{
			"target_name": "csdk",
			"type": "none",
			"actions": [ {
				"action_name": "build",
				"message": "Building C SDK",
				"outputs": [ "build/soletta_sysroot" ],
				"inputs": [],
				"action": [ "sh", "-c", "unset PYTHON; unset PYTHON_PATH; make alldefconfig; make all;" ]
			} ]
		},
		{
			"target_name": "collectbindings",
			"type": "none",
			"actions": [ {
				"action_name": "collectbindings",
				"message": "Collecting bindings",
				"outputs": ["bindings/nodejs/generated/main.cc"],
				"inputs": ["bindings/nodejs/generated/main.cc.prologue","bindings/nodejs/generated/main.cc.epilogue"],
				"action": [ "sh", "-c", "cd ./bindings/nodejs; ./generate-main.sh;" ]
			} ]
		},

		{
			"target_name": "soletta",
			"sources": [
				"bindings/nodejs/generated/main.cc",
				"bindings/nodejs/src/functions/dummy.cc"
			],
			"dependencies": [ "csdk", "collectbindings" ]
		}
	]
}
