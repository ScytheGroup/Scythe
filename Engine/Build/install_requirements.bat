@rem Script to install .NET SDK, .NET Runtime 6, Visual Studio, and C++ SDK
@rem Run this script with administrator privileges

@rem Install Visual Studio Community (latest version)
winget install Microsoft.VisualStudio.2022.Community --silent --override "--wait --quiet --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Component.VC.Llvm.Clang --add Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset --includeRecommended"

@rem Install .NET SDK
winget install Microsoft.DotNet.SDK.8 --silent

@rem Install .NET Runtime 6
winget install Microsoft.DotNet.Runtime.6 --silent

winget install Python.Python.3.13 --silent

winget install Git.Git --silent