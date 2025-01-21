using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShaderCompiler
{
    public static class Globals
    {
        public static volatile bool Terminate = false;
        public static int OriginalTaskCount = 0;
        public static int ProcessedTaskCount = 0;
        public static int FailedTaskCount = 0;
    }
}
