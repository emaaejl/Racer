#set( LLVM_LINK_COMPONENTS
#  ${LLVM_TARGETS_TO_BUILD}
#  Option
#  Support
#  )

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15.4" CACHE STRING "Minimum OS X deployment version" FORCE)

set( SOURCES
	steengaardPA.cpp
	steengaardPA.h
	macros.h
)

add_clang_executable(racer
  callGraphAnalysis.h	
  eventHandler.h	
  globalVarHandler.h
  globalVarHandler.cpp
  macros.h	
  racer.cpp
  raceAnalysis.h
  raceAnalysis.cpp
  steengaardPA.h
  steengaardPA.cpp
  steengaardPAVisitor.h
  steengaardPAVisitor.cpp
  flowSensitivePA.h
  flowSensitivePA.cpp
  symtab.h
  symtab.cpp
  symTabBuilder.h
  symTabBuilder.cpp  
  CGFrontendAction.h
  libExt/CompilerInstanceCtu.h
  libExt/CallGraphCtu.h
  libExt/CallGraphCtu.cpp
  )

#add_clang_executable(cgwpa
#	wpCallGraphBuilder.cpp
#)

target_link_libraries(racer
  LINK_PRIVATE clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangStaticAnalyzerFrontend
  clangTooling
  )


#add_custom_target(ast)
#foreach(Sourcefile ${SourceFiles})
#add_custom_command(TARGET ast PRE_BUILD
#       COMMAND clang -emit-ast ${SourceFile} -o /repo/emasabu/spa-test/${SourceFiles}.ast)

install(TARGETS racer
  RUNTIME DESTINATION bin)
