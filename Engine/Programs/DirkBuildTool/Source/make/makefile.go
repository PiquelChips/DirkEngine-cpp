package make

import (
	"bytes"
	"embed"
	"fmt"
)

type Makefile interface {
	ToBytes() ([]byte, error)
}

//go:embed makefiles
var makefiles embed.FS

func writeVar(buffer *bytes.Buffer, key string, values ...string) {
	buffer.WriteString(key)
	buffer.WriteString("=")
	if len(values) == 1 {
		buffer.WriteString(values[0])
	} else {
		for _, value := range values {
			buffer.WriteString(value)
			buffer.WriteString(" ")
		}
	}
	newLine(buffer)
}

func newLine(buffer *bytes.Buffer) { buffer.WriteString("\n") }

func writeBase(buffer *bytes.Buffer, name string) {
	data, err := makefiles.ReadFile(fmt.Sprintf("makefiles/%s.make", name))
	if err != nil {
		panic(err)
	}
	buffer.Write(data)
	newLine(buffer)
}

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

func (m *CppMakefile) ToBytes() ([]byte, error) {
	m.buffer = bytes.NewBuffer(nil)

	writeVar(m.buffer, "NAME", m.Name)

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
	newLine(m.buffer)

	writeVar(m.buffer, "ROOT_DIR", m.RootDir)
	writeVar(m.buffer, "BUILD_TYPE", m.BuildType)
	writeVar(m.buffer, "CFLAGS", m.CFlags)

	if m.Optimize {
		m.buffer.WriteString("CFLAGS+= -O3\n")
	}

	cxxFlags := []string{}
	for _, dir := range m.IncDirs {
		cxxFlags = append(cxxFlags, fmt.Sprintf("-I%s", dir))
	}
	writeVar(m.buffer, "CXXFLAGS", cxxFlags...)

	defines := []string{}
	for _, define := range m.Defines {
		defines = append(defines, fmt.Sprintf("-D%s", define))
	}
	writeVar(m.buffer, "DEFINES", defines...)

	writeVar(m.buffer, "LDFLAGS", m.LdFlags)

	ldLibs := []string{}
	for _, lib := range m.Libs {
		ldLibs = append(ldLibs, fmt.Sprintf("-l%s", lib))
	}
	writeVar(m.buffer, "LDLIBS", ldLibs...)

	writeBase(m.buffer, "base")
	writeBase(m.buffer, "compilation")

	if !m.IsLib && !m.IsStatic {
		// exec
		writeBase(m.buffer, "cxx_lnk")
	} else if !m.IsLib && m.IsStatic {
		// static exec
		writeBase(m.buffer, "cxx_lnk")
	} else if m.IsLib && !m.IsStatic {
		// shared lib
		m.buffer.WriteString("LDFLAGS+= -shared\n")
		writeBase(m.buffer, "cxx_lnk")
	} else if m.IsLib && m.IsStatic {
		// static lib
		writeBase(m.buffer, "ar_lnk")
	}

	return m.buffer.Bytes(), nil
}

type ShaderMakefile struct {
	Path    string
	RootDir string
	buffer  *bytes.Buffer
}

func (m *ShaderMakefile) ToBytes() ([]byte, error) {
	m.buffer = bytes.NewBuffer(nil)
	writeVar(m.buffer, "ROOT_DIR", m.RootDir)
	writeVar(m.buffer, "SHADER_DIR", m.Path)
	writeBase(m.buffer, "shaders")
	return m.buffer.Bytes(), nil
}
