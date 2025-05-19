package cp2p

import (
	cbase "github.com/jurgen-kluft/cbase/package"
	"github.com/jurgen-kluft/ccode/denv"
	centry "github.com/jurgen-kluft/centry/package"
	cunittest "github.com/jurgen-kluft/cunittest/package"
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
	mainlib := denv.SetupCppLibProject("cp2p", "github.com\\jurgen-kluft\\cp2p")
	mainlib.AddDependencies(basepkg.GetMainLib()...)

	// 'cp2p' unittest project
	maintest := denv.SetupDefaultCppTestProject("cp2p_test", "github.com\\jurgen-kluft\\cp2p")
	maintest.AddDependencies(cunittestpkg.GetMainLib()...)
	maintest.AddDependencies(entrypkg.GetMainLib()...)
	maintest.AddDependencies(basepkg.GetMainLib()...)
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
