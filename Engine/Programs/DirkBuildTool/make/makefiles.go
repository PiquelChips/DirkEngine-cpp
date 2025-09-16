package make

import (
	"bytes"
	_ "embed"
)

type Makefile struct {
	Name, Target     string
	RootDir, Type    string // exec, static, shared
	LibDirs, IncDirs []string
	Libs, Defines    []string
	LdFlags, CFlags  string
}

//go:embed base.make
var makeBase []byte

func (makefile *Makefile) ToBytes() ([]byte, error) {
	buffer := bytes.NewBuffer(nil)

	buffer.WriteString("NAME=")
	buffer.WriteString(makefile.Name)
	buffer.Write([]byte("\n"))

	buffer.WriteString("TARGET=")
	buffer.WriteString(makefile.Target)
	buffer.Write([]byte("\n"))

	buffer.WriteString("ROOT_DIR=")
	buffer.WriteString(makefile.RootDir)
	buffer.Write([]byte("\n"))

	buffer.WriteString("TYPE=")
	buffer.WriteString(makefile.Type)
	buffer.Write([]byte("\n"))

	buffer.WriteString("CFLAGS=")
	buffer.WriteString(makefile.CFlags)
	buffer.Write([]byte("\n"))

	buffer.WriteString("CXXFLAGS=")
	for _, dir := range makefile.IncDirs {
		buffer.WriteString(" -I")
		buffer.WriteString(dir)
	}
	buffer.Write([]byte("\n"))

	buffer.WriteString("DEFINES=")
	for _, define := range makefile.Defines {
		buffer.WriteString(" -D")
		buffer.WriteString(define)
	}
	buffer.Write([]byte("\n"))

	buffer.WriteString("LDFLAGS=")
	buffer.WriteString(makefile.LdFlags)
	for _, dir := range makefile.LibDirs {
		buffer.WriteString(" -L")
		buffer.WriteString(dir)
	}
	buffer.Write([]byte("\n"))

	buffer.WriteString("LDLIBS=")
	for _, lib := range makefile.Libs {
		buffer.WriteString(" -l")
		buffer.WriteString(lib)
	}
	buffer.Write([]byte("\n"))

	buffer.Write(makeBase)

	return buffer.Bytes(), nil
}
