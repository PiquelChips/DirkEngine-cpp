package config

import (
	"DirkBuildTool/output"
	"encoding/json"
	"fmt"
	"os"
)

type BuildType struct {
	Optimize bool     `json:"opmtimize"`
	Shipping bool     `json:"shipping"`
	Defines  []string `json:"defines"`
}

type Config struct {
	BuildTypes map[string]*BuildType `json:"build_configs"`
}

func LoadConfig() *Config {
	data, err := os.ReadFile(fmt.Sprintf("%s/config.json", output.Dirs.Config))
	if err != nil {
		fmt.Printf("Error loading config file: %s\n", err.Error())
		return nil
	}

	config := &Config{}
	if err := json.Unmarshal(data, &config); err != nil {
		fmt.Printf("Error parsing config file: %s\n", err.Error())
		return nil
	}

	if len(config.BuildTypes) < 1 {
		fmt.Printf("Config must have at least one build type\n")
		return nil
	}
	return config
}
