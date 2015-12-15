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
			"target_name": "collectbindings",
			"type": "none",
			"actions": [ {
				"action_name": "collectbindings",
				"message": "Collecting bindings",
				"outputs": ["generated/main.cc"],
				"inputs": ["generated/main.cc.prologue","generated/main.cc.epilogue"],
				"action": [ "sh", "./generate-main.sh" ]
			} ]
		},

		{
			"target_name": "soletta",
			"sources": [
				"generated/main.cc",
				"src/functions/dummy.cc"
			]
		}
	]
}
