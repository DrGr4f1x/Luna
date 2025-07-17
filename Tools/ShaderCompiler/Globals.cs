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
