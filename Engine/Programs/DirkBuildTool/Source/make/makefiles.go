package make

import (
	"bytes"
	"embed"
	"fmt"
)

type CppMakefile struct {
	Name, Target       string
	BuildType, RootDir string
	IncDirs            []string
	Libs, Defines      []string
	LdFlags, CFlags    string
	IsLib, IsStatic    bool
	Optimize           bool
	buffer             *bytes.Buffer
}

type Makefile interface {
	ToBytes() ([]byte, error)
	writeVar(key string, values ...string)
	newLine()
	writeBase(name string)
}

//go:embed makefiles
var makefiles embed.FS

func (m *CppMakefile) ToBytes() ([]byte, error) {
	m.buffer = bytes.NewBuffer(nil)

	m.writeVar("NAME", m.Name)

	m.buffer.WriteString("TARGET=")
	if m.IsLib {
		m.buffer.WriteString("lib")
	}
	m.buffer.WriteString(m.Target)
	if m.IsLib {
		if m.IsStatic {
			m.buffer.WriteString(".a")
		} else {
			m.buffer.WriteString(".so")
		}
	}
	m.newLine()

	m.writeVar("ROOT_DIR", m.RootDir)
	m.writeVar("BUILD_TYPE", m.BuildType)
	m.writeVar("CFLAGS", m.CFlags)

	if m.Optimize {
		m.buffer.WriteString("CFLAGS+= -O3\n")
	}

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

	m.writeVar("LDFLAGS", m.LdFlags)

	ldLibs := []string{}
	for _, lib := range m.Libs {
		ldLibs = append(ldLibs, fmt.Sprintf("-l%s", lib))
	}
	m.writeVar("LDLIBS", ldLibs...)

	m.writeBase("base")
	m.writeBase("compilation")

	if !m.IsLib && !m.IsStatic {
		// exec
		m.writeBase("cxx_lnk")
	} else if !m.IsLib && m.IsStatic {
		// static exec
		m.writeBase("cxx_lnk")
	} else if m.IsLib && !m.IsStatic {
		// shared lib
		m.buffer.WriteString("LDFLAGS+= -shared\n")
		m.writeBase("cxx_lnk")
	} else if m.IsLib && m.IsStatic {
		// static lib
		m.writeBase("ar_lnk")
	}

	return m.buffer.Bytes(), nil
}

func (m *CppMakefile) writeVar(key string, values ...string) {
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
	m.newLine()
}

func (m *CppMakefile) newLine() { m.buffer.WriteString("\n") }

func (m *CppMakefile) writeBase(name string) {
	data, err := makefiles.ReadFile(fmt.Sprintf("makefiles/%s.make", name))
	if err != nil {
		panic(err)
	}
	m.buffer.Write(data)
	m.newLine()
}
