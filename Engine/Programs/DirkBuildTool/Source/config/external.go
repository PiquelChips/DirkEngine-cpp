package config

import (
	"fmt"
	"os"
	"slices"
	"strings"
)

func setupExternal() ([]string, error) {
	externalLibs := []string{}
	if err := LoadConfigFile(externalLibFile, &externalLibs); err != nil {
		return nil, err
	}

	paths := getLibraryPaths(Settings.LibSearchEnvs)
	for _, lib := range externalLibs {
		for _, path := range paths {
			entries, err := os.ReadDir(path)
			if err != nil {
				continue
			}
			for _, entry := range entries {
				if entry.IsDir() {
					continue
				}

				if strings.HasPrefix(entry.Name(), fmt.Sprintf("lib%s.so", lib)) {
					os.Symlink(fmt.Sprintf("%s/%s", path, entry.Name()), fmt.Sprintf("%s/%s", Dirs.Binaries, entry.Name()))
				}
			}
		}
	}

	return externalLibs, nil
}

func getLibraryPaths(envVars []string) []string {
	paths := []string{}

	for _, envVar := range envVars {
		env := os.Getenv(envVar)
		for path := range strings.SplitSeq(env, ":") {
			if path == "" {
				continue
			}
			if slices.Contains(paths, path) {
				continue
			}

			paths = append(paths, path)
		}
	}

	return paths
}
