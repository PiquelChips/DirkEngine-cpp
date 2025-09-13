package build

import "fmt"

func Build() error {
	fmt.Printf("build has not been implemented yet. this is a work in progress\n")
	// detect & create bin & intermediate dirs
	// don't forget to add all include paths of deps to all modules

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
