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
						"inputs": [""],
						"action": [ "sh", "-c",
							"unset PYTHON && unset PYTHON_PATH && make alldefconfig && make"
						]
					} ]
				}
			]
		} ]
	],
	"targets": [
		{
			"target_name": "soletta",
			"sources": [ "soletta.cc" ],
			"conditions": [
				[ "'<!(echo $SOLETTA_FROM_MAKE)'!='true'", {
					"dependencies": [ "csdk" ]
				} ]
			]
		}
	]
}
