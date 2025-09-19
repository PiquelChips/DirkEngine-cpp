package make

import (
	"bytes"
	"embed"
	"fmt"
)

type Makefile struct {
	Name, Target     string
	RootDir          string
	LibDirs, IncDirs []string
	Libs, Defines    []string
	LdFlags, CFlags  string
	IsLib, IsStatic  bool
	Optimize         bool
	buffer           *bytes.Buffer
}

//go:embed makefiles
var makefiles embed.FS

func (m *Makefile) ToBytes() ([]byte, error) {
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
		m.buffer.WriteString("LDFLAGS += -shared\n")
		m.writeBase("cxx_lnk")
	} else if m.IsLib && m.IsStatic {
		// static lib
		m.writeBase("ar_lnk")
	}

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
	m.newLine()
}

func (m *Makefile) newLine() { m.buffer.WriteString("\n") }

func (m *Makefile) writeBase(name string) {
	data, err := makefiles.ReadFile(fmt.Sprintf("makefiles/%s.make", name))
	if err != nil {
		panic(err)
	}
	m.buffer.Write(data)
	m.newLine()
}
