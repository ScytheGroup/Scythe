using Sharpmake;
using System;
using System.IO;

namespace GameEngine;

public class Utils
{
    public class Reflector
    {
        private static string binaryPath => Path.Combine(Globals.BuildDirectory, "ThirdParty", "Reflector", "bin", "ScytheReflector");

        public static string Executable => binaryPath + (Util.GetExecutingPlatform() is Platform.win32 or Platform.win64 ? ".exe" : "");
    }
}