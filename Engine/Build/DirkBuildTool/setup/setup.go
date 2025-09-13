package setup

import (
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"encoding/json"
	"fmt"
	"path/filepath"
)

func Setup() error {
	// TODO: build glfw

	// hardcoded deps
	thirdparty := models.Thirdparty{
		"glm": &models.ThirdpartyDep{
			Name:         "glm",
			IsHeaderOnly: true,
			IncludeDir:   ".",
		},
		"tinygltf": &models.ThirdpartyDep{
			Name:         "tinygltf",
			IsHeaderOnly: true,
			IncludeDir:   ".",
		},
	}

	// make all paths absolute
	for _, dep := range thirdparty {
		dir, err := getDir(dep.Name)
		incDir, err := filepath.Abs(fmt.Sprintf("%s/%s", dir, dep.IncludeDir))
		if err != nil {
			return nil
		}
		dep.IncludeDir = incDir
	}

	// write the file
	data, err := json.Marshal(thirdparty)
	if err != nil {
		return nil
	}

	return output.WriteIntFile("thirdparty.json", data)
}

func getDir(name string) (string, error) {
	return filepath.Abs(fmt.Sprintf("%s/%s", output.Dirs.Thirdparty, name))
}
