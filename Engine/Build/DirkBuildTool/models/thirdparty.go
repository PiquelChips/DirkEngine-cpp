package models

type Thirdparty map[string]*ThirdpartyDep

// this will be genrated at setup, paths must be absolute
type ThirdpartyDep struct {
	Name         string `json:"name"`
	IsHeaderOnly bool   `json:"header_only"`
	IncludeDir   string `json:"inc_dir"`
	SrcDir       string `json:"src_dir,omitempty"`
}
