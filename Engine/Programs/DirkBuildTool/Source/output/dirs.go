package output

import (
	"os"
	"path/filepath"
)

const DirPerm = 0755

var Dirs struct {
	Root, Intermediate, Binaries,
	Source, Thirdparty, Config string
}

func GetOutDirs() error {
	if err := mkdirAndAbs(".", &Dirs.Root); err != nil {
		return err
	}
	if err := mkdirAndAbs("Intermediate", &Dirs.Intermediate); err != nil {
		return err
	}
	if err := mkdirAndAbs("Binaries", &Dirs.Binaries); err != nil {
		return err
	}
	if err := mkdirAndAbs("Engine/Source", &Dirs.Source); err != nil {
		return err
	}
	if err := mkdirAndAbs("Engine/Source/Thirdparty", &Dirs.Thirdparty); err != nil {
		return err
	}
	if err := mkdirAndAbs("Engine/Programs/DirkBuildTool/Config", &Dirs.Config); err != nil {
		return err
	}
	return nil
}

func mkdirAndAbs(dir string, out *string) error {
	dir, err := filepath.Abs(dir)
	if err != nil {
		return err
	}
	err = os.MkdirAll(dir, DirPerm)
	if err != nil {
		return err
	}
	(*out) = dir
	return nil
}
