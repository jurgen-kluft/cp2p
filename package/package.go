package cp2p

import (
	"github.com/jurgen-kluft/cbase/package"
	"github.com/jurgen-kluft/ccode/denv"
	"github.com/jurgen-kluft/centry/package"
	"github.com/jurgen-kluft/cunittest/package"
)

// GetPackage returns the package object of 'cp2p'
func GetPackage() *denv.Package {
	// Dependencies
	cunittestpkg := cunittest.GetPackage()
	entrypkg := centry.GetPackage()
	basepkg := cbase.GetPackage()

	// The main (cp2p) package
	mainpkg := denv.NewPackage("cp2p")
	mainpkg.AddPackage(cunittestpkg)
	mainpkg.AddPackage(entrypkg)
	mainpkg.AddPackage(basepkg)

	// 'cp2p' library
	mainlib := denv.SetupDefaultCppLibProject("cp2p", "github.com\\jurgen-kluft\\cp2p")
	mainlib.Dependencies = append(mainlib.Dependencies, basepkg.GetMainLib())

	// 'cp2p' unittest project
	maintest := denv.SetupDefaultCppTestProject("cp2p_test", "github.com\\jurgen-kluft\\cp2p")
	maintest.Dependencies = append(maintest.Dependencies, cunittestpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, entrypkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, basepkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
