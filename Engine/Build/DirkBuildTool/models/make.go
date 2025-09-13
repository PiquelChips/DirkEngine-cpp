package models

type Makefile struct {
	Target, RootDir  string
	Type             string
	LibDirs, IncDirs []string
	Libs, Defines    []string
	LdFlags, CFlags  string
}
