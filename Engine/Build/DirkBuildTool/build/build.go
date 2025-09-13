package build

import (
	"DirkBuildTool/setup"
	"fmt"
)

func Build() error {
	fmt.Printf("build has not been implemented yet. this is a work in progress\n")
	thirdparty, err := setup.ReadThirdparty()
	if err != nil {
		return err
	}

	/**
	 * BUILD OPTIONS:
	 * - shipping -- static linking & all optimizations
	 * - dev -- shared linking & no optimizations
	 */

	/**
	 * BUILD FLOW
	 * - load saved thirdparty dependencies
	 * - look for target in src dir (for now will be Editor)
	 * - resolve dependencies (ignore duplicates)
	 * - build every target
	 *   - main target should be last (Editor)
	 *   - generate makefile & run
	 */
	return nil
}
