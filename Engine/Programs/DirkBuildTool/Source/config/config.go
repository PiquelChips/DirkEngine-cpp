package config

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
)

type BuildType struct {
	Name     string   `json:"-"`
	Optimize bool     `json:"optimize"`
	Compact  bool     `json:"compact"` // compact the output (essentially statically linking)
	Defines  []string `json:"defines"`
}

type DirsConfig struct {
	Root         string   `json:"-"`
	Intermediate string   `json:"intermediate"`
	Binaries     string   `json:"binaries"`
	Thirdparty   string   `json:"thirdparty"`
	Modules      []string `json:"modules"` // dirs that will be searched for modules
}

var BuildTypes map[string]*BuildType
var Dirs DirsConfig

const configDir = "Engine/Programs/DirkBuildTool/Config"

func LoadConfig() {
	log.Printf("Loading configuration\n")
	Dirs = loadDirsConfig()
	BuildTypes = loadBuildTypes()
}

func loadBuildTypes() map[string]*BuildType {
	buildTypes := map[string]*BuildType{}
	if err := loadConfig("build_configs.json", &buildTypes); err != nil {
		fmt.Printf("%s\n", err.Error())
		os.Exit(1)
		return nil
	}

	if len(buildTypes) < 1 {
		fmt.Printf("Config must have at least one build type\n")
		os.Exit(1)
		return nil
	}

	for name, conf := range buildTypes {
		conf.Name = name
	}

	return buildTypes
}

func loadDirsConfig() DirsConfig {
	dirs := DirsConfig{}
	if err := loadConfig("dirs.json", &dirs); err != nil {
		fmt.Printf("%s\n", err.Error())
		os.Exit(1)
		return DirsConfig{}
	}

	dirs.Root, _ = filepath.Abs(".")
	dirs.Intermediate, _ = filepath.Abs(dirs.Intermediate)
	dirs.Binaries, _ = filepath.Abs(dirs.Binaries)
	dirs.Thirdparty, _ = filepath.Abs(dirs.Thirdparty)
	for i, module := range dirs.Modules {
		dirs.Modules[i], _ = filepath.Abs(module)
	}

	const DirPerm = 0755

	if err := os.MkdirAll(dirs.Intermediate, DirPerm); err != nil {
		panic(err)
	}
	if err := os.MkdirAll(dirs.Binaries, DirPerm); err != nil {
		panic(err)
	}

	return dirs
}

func loadConfig(file string, out any) error {
	data, err := os.ReadFile(fmt.Sprintf("%s/%s", configDir, file))
	if err != nil {
		return fmt.Errorf("Error loading config file: %w", err)
	}

	if err := json.Unmarshal(data, out); err != nil {
		return fmt.Errorf("Error parsing config file: %w", err)
	}

	return nil
}
