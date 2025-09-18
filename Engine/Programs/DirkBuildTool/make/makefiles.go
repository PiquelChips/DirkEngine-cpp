package make

import (
	"bytes"
	"embed"
	"fmt"
)

type Makefile struct {
	Name, Target     string
	RootDir, Type    string // exec, static, shared
	LibDirs, IncDirs []string
	Libs, Defines    []string
	LdFlags, CFlags  string
	buffer           *bytes.Buffer
}

//go:embed base.make
var makeBase []byte

//go:embed makefiles
var makefiles embed.FS

func (m *Makefile) ToBytes() ([]byte, error) {
	m.buffer = bytes.NewBuffer(nil)

	m.writeVar("NAME", m.Name)
	m.writeVar("TARGET", m.Target)
	m.writeVar("ROOT_DIR", m.RootDir)
	m.writeVar("TYPE", m.Type)
	m.writeVar("CFLAGS", m.CFlags)

	cxxFlags := []string{}
	for _, dir := range m.IncDirs {
		cxxFlags = append(cxxFlags, fmt.Sprintf("-I%s", dir))
	}
	m.writeVar("CXXFLAGS", cxxFlags...)

	defines := []string{}
	for _, define := range m.Defines {
		defines = append(defines, fmt.Sprintf("-D%s", define))
	}
	m.writeVar("DEFINES", defines...)

	ldFlags := []string{m.LdFlags}
	for _, dir := range m.LibDirs {
		ldFlags = append(ldFlags, fmt.Sprintf("-L%s", dir))
	}
	m.writeVar("LDFLAGS", ldFlags...)

	ldLibs := []string{}
	for _, lib := range m.Libs {
		ldLibs = append(ldLibs, fmt.Sprintf("-l%s", lib))
	}
	m.writeVar("LDLIBS", ldLibs...)

	m.buffer.Write(makeBase)

	return m.buffer.Bytes(), nil
}

func (m *Makefile) writeVar(key string, values ...string) {
	m.buffer.WriteString(key)
	m.buffer.WriteString("=")
	if len(values) == 1 {
		m.buffer.WriteString(values[0])
	} else {
		for _, value := range values {
			m.buffer.WriteString(value)
			m.buffer.WriteString(" ")
		}
	}
	m.buffer.WriteString("\n")
}
