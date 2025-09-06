using Microsoft.Win32;
using System.Collections;
using System.Collections.Concurrent;
using System.CommandLine;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
    enum Platform
    {
        DXBC,
        DXIL,
        SPIRV
    }

    class Options
    {
        #region Required options
        public string? PlatformName { get; set; } = "";
        public string? ConfigFile { get; set; } = "";
        public string? OutputDir { get; set; } = "";
        public bool Binary { get; set; } = false;
        public bool Header { get; set; } = false;
        public bool BinaryBlob { get; set; } = false;
        public bool HeaderBlob { get; set; } = false;
        #endregion

        #region Compiler settings
        public string? ShaderModel { get; set; } = "6_5";
        public uint OptimizationLevel { get; set; } = 3;
        public bool WarningsAreErrors { get; set; } = false;
        public bool AllResourcesBound { get; set; } = false;
        public bool PDB { get; set; } = false;
        public bool EmbedPDB { get; set; } = false;
        public bool StripReflection { get; set; } = false;
        public bool MatrixRowMajor { get; set; } = false;
        public bool Hlsl2021 { get; set; } = false;
        public string? VulkanMemoryLayout { get; set; } = "";
        public string[]? CompilerOptions { get; set; } = Array.Empty<string>();
        #endregion

        #region Defines and includes
        public string[]? IncludeDirs { get; set; } = Array.Empty<string>();
        public string[]? Defines { get; set; } = Array.Empty<string>();
        #endregion

        #region Other options
        public bool Force { get; set; } = false;
        public string? SourceDir { get; set; } = "";
        public string? Compiler { get; set; } = "";
        public string[]? RelaxedIncludes { get; set; } = Array.Empty<string>();
        public string? OutputExt { get; set; }
        public bool Serial { get; set; } = false;
        public bool Flatten { get; set; } = false;
        public bool Continue { get; set; } = false;
        public bool Verbose { get; set; } = false;
        public int RetryCount { get; set; } = 10;
        #endregion

        #region SPIRV options
        public string? VulkanVersion { get; set; } = "1.3";
        public string[]? SpirvExt { get; set; } = Array.Empty<string>();
        public int SRegShift { get; set; } = 100;
        public int TRegShift { get; set; } = 200;
        public int BRegShift { get; set; } = 300;
        public int URegShift { get; set; } = 400;
        public bool NoRegShifts { get; set; } = false;
        #endregion

        public Platform Platform { get; set; }
        public string Self { get; set; }

        public bool IsBlob { get { return BinaryBlob || HeaderBlob; } }

        public bool Parse(string[] args)
        {
            Self = args[0];

            #region Required options
            var platformOpt = new Option<string>("--platform", "DXBC, DXIL, or SPIRV").FromAmong("DXBC", "DXIL", "SPIRV");
            platformOpt.IsRequired = true;
            platformOpt.AddAlias("-p");

            var configFileOpt = new Option<string>("--config", "Configuration file with the list of shaders to compile");
            configFileOpt.IsRequired = true;
            configFileOpt.AddAlias("-c");

            var outputDirOpt = new Option<string>("--out", "Output directory");
            outputDirOpt.IsRequired = true;
            outputDirOpt.AddAlias("-o");

            var binaryOpt = new Option<bool>("--binary", "Output binary files");
            binaryOpt.AddAlias("-b");

            var headerOpt = new Option<bool>("--header", "Output header files");
            headerOpt.AddAlias("-h");

            var binaryBlobOpt = new Option<bool>("--binaryBlob", "Output binary blob files");
            binaryBlobOpt.AddAlias("-B");

            var headerBlobOpt = new Option<bool>("--headerBlob", "Output header blob files");
            headerBlobOpt.AddAlias("-H");
            #endregion

            #region Compiler options
            var shaderModelOpt = new Option<string>("--shaderModel", "Shader model for DXIL/SPIRV");
            shaderModelOpt.AddAlias("-m");

            var optimizationOpt = new Option<uint>("--optimization", "Optimization level 03 (default = 3, disabled = 0)");
            optimizationOpt.AddAlias("-O");

            var warningsAreErrorsOpt = new Option<bool>("--WX", "Maps to '-WX' DXC/FXC option: warnings are errors");
            var allResourcesBoundOpt = new Option<bool>("--allResourcesBound", "Maps to -all_resources_bound DXC/FXC option: all resources bound");
            var pdbOpt = new Option<bool>("--PDB", "Output PDB files in 'out/PDB' folder");
            var embedPDBOpt = new Option<bool>("--embedPDB", "Embed PDB with shader binary");
            var stripReflectionOpt = new Option<bool>("--stripReflection", "Maps to '-Qstrip_reflect' DXC/FXC option: strip reflection information from a shader binary");
            var matrixRowMajorOpt = new Option<bool>("--matrixRowMajor", "Maps to '-Zpr' DXC/FXC option: pack matrices in row-major order");
            var hlsl2021Opt = new Option<bool>("--hlsl2021", "Maps to '-HV 2021' DXC option: enable HLSL 2021 standard");
            var vulkanMemoryLayoutOpt = new Option<string>("--vulkanMemoryLayout", "Maps to '-fvk-use-<VALUE>-layout' DXC options: dx, gl, scalar");

            var compilerOptionsOpt = new Option<string[]>("--compilerOptions", "Custom command line options for the compiler, separated by spaces");
            compilerOptionsOpt.AddAlias("-X");
            #endregion

            #region Defines and includes
            var includeOpt = new Option<string[]>("--include", "Include directory(s)");
            includeOpt.AddAlias("-I");

            var defineOpt = new Option<string[]>("--define", "Macro definition(s) in forms 'M=value' or 'M'");
            defineOpt.AddAlias("-D");
            #endregion

            #region Other options
            var forceOpt = new Option<bool>("--force", "Treat all source files as modified");
            forceOpt.AddAlias("-f");

            var sourceDirOpt = new Option<string>("--sourceDir", "Source code directory");
            var compilerOpt = new Option<string>("--compiler", "Path to a DXC or FXC executable");
            var relaxedIncludeOpt = new Option<string[]>("--relaxedInclude", "Include file(s) not invoking re-compilation");
            var outputExtOpt = new Option<string>("--outputExt", "Extension for output files, default is one of .dxbc, .dxil, or .spirv");
            var serialOpt = new Option<bool>("--serial", "Disable multi-threading");
            var flattenOpt = new Option<bool>("--flatten", "Flatten source directory structure in the output directory");
            var continueOnErrorOpt = new Option<bool>("--continue", "Continue compilation if an error occurred");
            var verboseOpt = new Option<bool>("--verbose", "Print commands before they are executed");
            var retryCountOpt = new Option<int>("--retryCount", "Retry count for compilation task sub-process failures");
            #endregion

            #region SPIRV options
            var vulkanVersionOpt = new Option<string>("--vulkanVersion", "Vulkan environment version, maps t0 '-fspv-target-env' (default  = 1.3)");
            var spirvExtOpt = new Option<string[]>("--spirvExt", "Maps to -fspv-extension' option: add SPIR-V extension permitted to use");
            var sRegShiftOpt = new Option<int>("--sRegShift", "SPIRV: register shift for sampler (s#) resources");
            var tRegShiftOpt = new Option<int>("--tRegShift", "SPIRV: register shift for texture (t#) resources");
            var bRegShiftOpt = new Option<int>("--bRegShift", "SPIRV: register shift for constant (b#) resources");
            var uRegShiftOpt = new Option<int>("--uRegShift", "SPIRV: register shift for UAV (u#) resources");
            var noRegShiftsOpt = new Option<bool>("--noRegShifts", "Don't specify any register shifts for the compiler");
            #endregion

            // Setup root command
            var rootCommand = new RootCommand("ShaderCompiler");
            rootCommand.AddOption(platformOpt);
            rootCommand.AddOption(configFileOpt);
            rootCommand.AddOption(outputDirOpt);
            rootCommand.AddOption(binaryOpt);
            rootCommand.AddOption(headerOpt);
            rootCommand.AddOption(binaryBlobOpt);
            rootCommand.AddOption(headerBlobOpt);

            rootCommand.AddOption(shaderModelOpt);
            rootCommand.AddOption(optimizationOpt);
            rootCommand.AddOption(warningsAreErrorsOpt);
            rootCommand.AddOption(allResourcesBoundOpt);
            rootCommand.AddOption(pdbOpt);
            rootCommand.AddOption(embedPDBOpt);
            rootCommand.AddOption(stripReflectionOpt);
            rootCommand.AddOption(matrixRowMajorOpt);
            rootCommand.AddOption(hlsl2021Opt);
            rootCommand.AddOption(vulkanMemoryLayoutOpt);
            rootCommand.AddOption(compilerOptionsOpt);

            rootCommand.AddOption(includeOpt);
            rootCommand.AddOption(defineOpt);

            rootCommand.AddOption(forceOpt);
            rootCommand.AddOption(sourceDirOpt);
            rootCommand.AddOption(compilerOpt);
            rootCommand.AddOption(relaxedIncludeOpt);
            rootCommand.AddOption(outputExtOpt);
            rootCommand.AddOption(serialOpt);
            rootCommand.AddOption(flattenOpt);
            rootCommand.AddOption(continueOnErrorOpt);
            rootCommand.AddOption(verboseOpt);
            rootCommand.AddOption(retryCountOpt);

            rootCommand.AddOption(vulkanVersionOpt);
            rootCommand.AddOption(spirvExtOpt);
            rootCommand.AddOption(sRegShiftOpt);
            rootCommand.AddOption(tRegShiftOpt);
            rootCommand.AddOption(bRegShiftOpt);
            rootCommand.AddOption(uRegShiftOpt);
            rootCommand.AddOption(noRegShiftsOpt);

            rootCommand.SetHandler(async (context) =>
            {
                // Required options
                PlatformName = context.ParseResult.GetValueForOption(platformOpt);
                ConfigFile = context.ParseResult.GetValueForOption(configFileOpt);
                OutputDir = context.ParseResult.GetValueForOption(outputDirOpt);
                Binary = context.ParseResult.GetValueForOption(binaryOpt);
                Header = context.ParseResult.GetValueForOption(headerOpt);
                BinaryBlob = context.ParseResult.GetValueForOption(binaryBlobOpt);
                HeaderBlob = context.ParseResult.GetValueForOption(headerBlobOpt);

                // Compiler options
                ShaderModel = context.ParseResult.GetValueForOption(shaderModelOpt);
                OptimizationLevel = context.ParseResult.GetValueForOption(optimizationOpt);
                WarningsAreErrors = context.ParseResult.GetValueForOption(warningsAreErrorsOpt);
                AllResourcesBound = context.ParseResult.GetValueForOption(allResourcesBoundOpt);
                PDB = context.ParseResult.GetValueForOption(pdbOpt);
                EmbedPDB = context.ParseResult.GetValueForOption(embedPDBOpt);
                StripReflection = context.ParseResult.GetValueForOption(stripReflectionOpt);
                MatrixRowMajor = context.ParseResult.GetValueForOption(matrixRowMajorOpt);
                Hlsl2021 = context.ParseResult.GetValueForOption(hlsl2021Opt);
                VulkanMemoryLayout = context.ParseResult.GetValueForOption(vulkanMemoryLayoutOpt);
                CompilerOptions = context.ParseResult.GetValueForOption(compilerOptionsOpt);

                // Defines and includes
                IncludeDirs = context.ParseResult.GetValueForOption(includeOpt);
                Defines = context.ParseResult.GetValueForOption(defineOpt);

                // Other options
                Force = context.ParseResult.GetValueForOption(forceOpt);
                SourceDir = context.ParseResult.GetValueForOption(sourceDirOpt);
                Compiler = context.ParseResult.GetValueForOption(compilerOpt);
                RelaxedIncludes = context.ParseResult.GetValueForOption(relaxedIncludeOpt);
                OutputExt = context.ParseResult.GetValueForOption(outputExtOpt);
                Serial = context.ParseResult.GetValueForOption(serialOpt);
                Flatten = context.ParseResult.GetValueForOption(flattenOpt);
                Continue = context.ParseResult.GetValueForOption(continueOnErrorOpt);
                Verbose = context.ParseResult.GetValueForOption(verboseOpt);
                RetryCount = context.ParseResult.GetValueForOption(retryCountOpt);

                // SPIRV options
                VulkanVersion = context.ParseResult.GetValueForOption(vulkanVersionOpt);
                SpirvExt = context.ParseResult.GetValueForOption(spirvExtOpt);
                SRegShift = context.ParseResult.GetValueForOption(sRegShiftOpt);
                TRegShift = context.ParseResult.GetValueForOption(tRegShiftOpt);
                BRegShift = context.ParseResult.GetValueForOption(bRegShiftOpt);
                URegShift = context.ParseResult.GetValueForOption(uRegShiftOpt);
                NoRegShifts = context.ParseResult.GetValueForOption(noRegShiftsOpt);
            });

            rootCommand.Invoke(args);

            return Validate();
        }

        public bool Validate()
        {
            // Check that the config file exists
            if ((ConfigFile is not null) && !File.Exists(ConfigFile))
            {
                System.Console.Error.WriteLine("ERROR: Config file {0} does not exist!", ConfigFile);
                return false;
            }

            // Create output directory if it does not exist
            if ((OutputDir is not null) && !Directory.Exists(OutputDir))
            {
                try
                {
                    Directory.CreateDirectory(OutputDir);
                }
                catch (IOException ex)
                {
                    System.Console.Error.WriteLine("ERROR: Could not create output directory {0}!  Exception: {1}.", OutputDir, ex.Message);
                    return false;
                }
            }

            // Check output type
            if (!Binary && !Header && !BinaryBlob && !HeaderBlob)
            {
                System.Console.Error.WriteLine("ERROR: One of 'binary', 'header', 'binaryBlob', or 'headerBlob' must be set!");
                return false;
            }

            // Check compiler
            if ((Compiler is not null) && !File.Exists(Compiler))
            {
                System.Console.Error.WriteLine("ERROR: Compiler {0} does not exist!", Compiler);
                return false;
            }

            // Get platform
            if (PlatformName == "DXBC")
                Platform = Platform.DXBC;
            else if (PlatformName == "DXIL")
                Platform = Platform.DXIL;
            else if (PlatformName == "SPIRV")
                Platform = Platform.SPIRV;
            else
            {
                System.Console.Error.WriteLine("ERROR: Unknown platform {0}", PlatformName);
                return false;
            }

            // Fixup output extension
            if ((OutputExt is not null) && !OutputExt.StartsWith("."))
            {
                OutputExt = "." + OutputExt;
            }

            return true;
        }
    }

    class DirectoryComparer : IComparer
    {
        public int Compare(Object x, Object y)
        {
            return (new CaseInsensitiveComparer()).Compare(((DirectoryInfo)y).FullName, ((DirectoryInfo)x).FullName);
        }
    }

    class CompileJob
    {
        public string[]? Defines { get; set; }
        public string? Source { get; set; }
        public string? EntryPoint { get; set; }
        public string? Profile { get; set; }
        public string? OutputFileWithoutExt { get; set; }
        public string? CombinedDefines { get; set; }
        public uint OptimizationLevel { get; set; } = 3;
    }

    class BlobEntry
    {
        public string PermutationFileWithoutExt { get; set; } = "";
        public string CombinedDefines { get; set; } = "";
    }

    class ConfigLine
    {
        public string Source { get; set; } = "";
        public string[]? Defines { get; set; } = Array.Empty<string>();
        public string? Entry { get; set; } = "main";
        public string? Profile { get; set; } = "";
        public string? OutputDir { get; set; } = "";
        public string? OutputSuffix { get; set; } = "";
        public uint OptimizationLevel { get; set; } = 0xFFFFFFFF;

        public bool Parse(string[] args)
        {
            var profileOpt = new Option<string>("--profile", "Shader profile");
            profileOpt.AddAlias("-T");

            var entryPointOpt = new Option<string>("--entryPoint", "(Optional) entry point");
            entryPointOpt.AddAlias("-E");

            var defineOpt = new Option<string[]>("--define", "(Optional) define(s) in forms 'M=value' or 'M'");
            defineOpt.AddAlias("-D");

            var outputOpt = new Option<string>("--output", "(Optional) output subdirectory");
            outputOpt.AddAlias("-o");

            var optimizationOpt = new Option<uint>("--optimization", "(Optional) optimization level");
            optimizationOpt.AddAlias("-O");
            optimizationOpt.SetDefaultValue(0xFFFFFFFF);

            var outputSuffixOpt = new Option<string>("--outputSuffix", "(Optional) suffix to add before extension after filename");

            var command = new Command("ConfigLine");
            command.AddOption(profileOpt);
            command.AddOption(entryPointOpt);
            command.AddOption(defineOpt);
            command.AddOption(outputOpt);
            command.AddOption(optimizationOpt);
            command.AddOption(outputSuffixOpt);

            this.Source = args[0];
            string[] trimmedArgs = new string[args.Length - 1];
            Array.Copy(args, 1, trimmedArgs, 0, args.Length - 1);

            command.SetHandler(async (context) =>
            {
                // Required options
                this.Profile = context.ParseResult.GetValueForOption(profileOpt);
                this.Entry = context.ParseResult.GetValueForOption(entryPointOpt);
                this.Defines = context.ParseResult.GetValueForOption(defineOpt);
                this.OutputDir = context.ParseResult.GetValueForOption(outputOpt);
                this.OptimizationLevel = context.ParseResult.GetValueForOption(optimizationOpt);
                this.OutputSuffix = context.ParseResult.GetValueForOption(outputSuffixOpt);
            });

            command.Invoke(trimmedArgs);

            if (Profile is null)
            {
                System.Console.Error.WriteLine("ERROR: Shader target not specified!");
                return false;
            }

            return true;
        }
    }

    class CompileErrorHandler
    {
        private string m_shaderCompError = "";
        private string m_shaderCompOutput = "";

        public string Error { get { return m_shaderCompError; } }
        public string Output { get { return m_shaderCompOutput; } }

        public void StdErrorHandler(object sender, DataReceivedEventArgs args)
        {
            string? message = args.Data;

            if (message is null)
                return;
            if (message.Length > 0)
            {
                m_shaderCompError += message + "\n";
            }
        }

        public void StdOutputHandler(object sender, DataReceivedEventArgs args)
        {
            string? message = args.Data;

            if (message is null)
                return;
            if (message.Length > 0)
            {
                m_shaderCompOutput += message + "\n";
            }
        }
    }

    class DataWriter
    {
        private FileStream? m_fileStream;
        private bool m_textMode;
        private int m_lineLength = 129;
        private bool m_isFileOpen = false;

        public bool IsFileOpen { get { return m_isFileOpen; } }

        public DataWriter(string file, bool textMode)
        {
            m_textMode = textMode;
            try
            {
                m_fileStream = File.OpenWrite(file);
                m_isFileOpen = true;
            }
            catch
            {
                m_isFileOpen = false;
            }

        }

        public void WriteTextProlog(string shaderName)
        {
            string prolog = string.Format("const uint8_t {0}[] = {{", shaderName);
            WriteString(prolog);
        }

        public void WriteTextEpilog()
        {
            string epilog = "\n}};\n";
            WriteString(epilog);
        }

        public bool WriteDataAsBinary(byte[] data)
        {
            if (m_fileStream is null) return false;

            if (data.Length == 0)
                return true;

            try
            {
                m_fileStream.Write(data, 0, data.Length);
            }
            catch
            {
                return false;
            }

            return true;
        }

        public bool WriteDataAsText(byte[] data)
        {
            for (uint i = 0; i < data.Length; ++i)
            {
                byte value = data[i];

                if (m_lineLength > 128)
                {
                    WriteString("\n    ");
                    m_lineLength = 0;
                }

                string str = string.Format("{0}, ", value);
                WriteString(str);

                if (value < 10)
                    m_lineLength += 3;
                else if (value < 100)
                    m_lineLength += 4;
                else
                    m_lineLength += 5;
            }

            return true;
        }

        private void WriteString(string str)
        {
            if (m_fileStream is null) return;

            byte[] data = Encoding.UTF8.GetBytes(str);
            m_fileStream.Write(data, 0, data.Length);
        }
    }

    struct ShaderBlobEntry
    {
        public uint permutationSize;
        public uint dataSize;
    }

    class Compiler
    {
        private string? m_vulkanDxcCompiler = null;
        private string? m_windowsDxcCompiler = null;
        private string? m_windowsFxcCompiler = null;

        private Dictionary<string, DateTime> m_hierarchicalUpdateTimes = new();
        private Dictionary<string, List<BlobEntry>> m_shaderBlobs = new();
        private Regex m_includePattern = new("\\s*#include\\s+[\"<]([^>\"]+)[>\"].*");
        private ConcurrentQueue<CompileJob> m_jobQueue = new();
        private Mutex m_progressMutex = new();
        public Options Options { get; set; }

        public Compiler(Options options)
        {
            this.Options = options;

            FindVulkanDxcCompiler();
            FindWindowsCompilers();
        }

        public void FindVulkanDxcCompiler()
        {
            string? vulkanSdkPath = System.Environment.GetEnvironmentVariable("VULKAN_SDK");
            if (vulkanSdkPath is not null)
            {
                m_vulkanDxcCompiler = Path.Combine(vulkanSdkPath, "Bin\\dxc.exe");
            }
        }

        public RegistryKey? FindLocalMachineKey(string path)
        {
            string[] subs = path.Split('\\');
            RegistryKey? key = Registry.LocalMachine;
            foreach (var subpath in subs)
            {
                key = key.OpenSubKey(subpath);
                if (key == null)
                {
                    break;
                }
            }
            return key;
        }

        public void FindWindowsCompilers()
        {
            string installedRootsPath = "SOFTWARE\\WOW6432Node\\Microsoft\\Windows Kits\\Installed Roots";
            RegistryKey? installedRootsKey = FindLocalMachineKey(installedRootsPath);

            if (installedRootsKey != null)
            {
                var kitsRootDirectory = new DirectoryInfo((string)installedRootsKey.GetValue("KitsRoot10"));

                if (kitsRootDirectory.Exists)
                {
                    var binDirectory = new DirectoryInfo(kitsRootDirectory.FullName + "\\bin");
                    if (binDirectory.Exists)
                    {
                        var searchDirectories = binDirectory.GetDirectories("10.0.*");

                        Array.Sort(searchDirectories, new DirectoryComparer());
                        searchDirectories.Reverse();

                        bool bFoundDxc = false;
                        bool bFoundFxc = false;

                        foreach (var dir in searchDirectories)
                        {
                            if (bFoundDxc && bFoundFxc)
                                break;

                            var x64Dir = new DirectoryInfo(dir.FullName + "\\x64");
                            if (!x64Dir.Exists)
                                continue;

                            var files = x64Dir.GetFiles("*.exe");
                            foreach (var fi in files)
                            {
                                if (!bFoundDxc && fi.Name == "dxc.exe")
                                {
                                    m_windowsDxcCompiler = fi.FullName;
                                    bFoundDxc = true;
                                }

                                if (!bFoundFxc && fi.Name == "fxc.exe")
                                {
                                    m_windowsFxcCompiler = fi.FullName;
                                    bFoundFxc = true;
                                }

                                if (bFoundDxc && bFoundFxc)
                                    break;
                            }
                        }
                    }
                }
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Error.WriteLine("Could not find expected registry key {0}", installedRootsPath);
                Console.ResetColor();
            }
        }

        public string GetOutputExtension()
        {
            if (Options.OutputExt is not null)
            {
                return Options.OutputExt;
            }
            else
            {
                return GetDefaultExtension(Options.Platform);
            }
        }

        public string? GetCompiler()
        {
            if (Options.Compiler is not null)
            {
                return Options.Compiler;
            }

            switch (Options.Platform)
            {
                case Platform.DXBC:
                    return m_windowsFxcCompiler;

                case Platform.DXIL:
                    return m_windowsDxcCompiler;

                case Platform.SPIRV:
                    return m_vulkanDxcCompiler;
            }

            return null;
        }

        public bool ValidateCompiler()
        {
            string? compiler = GetCompiler();
            if (compiler is not null && File.Exists(compiler))
                return true;
            return false;
        }

        public static T Max<T>(T first, T second)
        {
            if (Comparer<T>.Default.Compare(first, second) > 0)
                return first;
            return second;
        }


        public static T Min<T>(T first, T second)
        {
            if (Comparer<T>.Default.Compare(first, second) < 0)
                return first;
            return second;
        }

        public static string EscapePath(string path)
        {
            if (path.Contains(' '))
            {
                return "\"" + path + "\"";
            }
            return path;
        }

        public static string GetShaderName(string path, Platform platform)
        {
            string name = path;
            name.Replace('.', '_');
            name += "_" + GetDefaultExtension(platform).Substring(1);

            return "g_" + name;
        }

        public static string GetDefaultExtension(Platform platform)
        {
            switch (platform)
            {
                case Platform.DXBC:
                    return ".dbxc";

                case Platform.DXIL:
                    return ".dxil";

                default:
                    return ".spirv";
            }
        }

        public static string GetParentPath(string filename)
        {
            for (int i = filename.Length - 1; i >= 0; --i)
            {
                if (filename[i] == '/' || filename[i] == '\\')
                {
                    return filename.Substring(0, i);
                }
            }
            return "";
        }

        public string TrimConfigLine(string line)
        {
            // Remove leading and trailing whitespace
            line.Trim();

            // Replace tabs with space
            line.Replace("\t", " ");

            // Remove double spaces
            line.Replace("  ", " ");

            return line;
        }

        public bool GetHierarchicalUpdateTime(string file, LinkedList<string> callStack, out DateTime outTime)
        {
            DateTime value;
            if (m_hierarchicalUpdateTimes.TryGetValue(file, out value))
            {
                outTime = value;
                return true;
            }

            FileInfo fi = new(file);
            try
            {
                var streamReader = fi.OpenText();

                callStack.AddFirst(file);
                string path = System.IO.Directory.GetParent(file).FullName;
                DateTime hierarchicalUpdateTime = File.GetLastWriteTime(file);

                string? line;
                while ((line = streamReader.ReadLine()) is not null)
                {
                    string includeName = "";
                    bool bHasMatch = false;
                    foreach (Match match in m_includePattern.Matches(line))
                    {
                        includeName = match.Groups[1].Value;
                        bHasMatch = true;
                        break;
                    }

                    if (!bHasMatch)
                        continue;


                    bool bFoundRelaxedInclude = false;
                    if (Options.RelaxedIncludes is not null)
                    {
                        foreach (var relaxedInclude in Options.RelaxedIncludes)
                        {
                            if (relaxedInclude == includeName)
                            {
                                bFoundRelaxedInclude = true;
                                break;
                            }
                        }
                    }

                    if (bFoundRelaxedInclude)
                        continue;

                    bool isFound = false;
                    string includeFile = Path.Combine(path, includeName);
                    if (File.Exists(includeFile))
                    {
                        isFound = true;
                    }
                    else
                    {
                        if (Options.IncludeDirs is not null)
                        {
                            foreach (var includePath in Options.IncludeDirs)
                            {
                                includeFile = Path.Combine(includePath, includeName);
                                if (File.Exists(includeFile))
                                {
                                    isFound = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (!isFound)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.Error.WriteLine("ERROR: Can't find include file {0}, included in:", includeName);
                        foreach (var item in callStack)
                        {
                            Console.Error.WriteLine("\t{0}", item);
                        }
                        Console.ResetColor();

                        outTime = DateTime.MinValue;
                        return false;
                    }

                    DateTime dependencyTime;
                    if (!GetHierarchicalUpdateTime(includeFile, callStack, out dependencyTime))
                    {
                        outTime = DateTime.MinValue;
                        return false;
                    }

                    hierarchicalUpdateTime = Max<DateTime>(dependencyTime, hierarchicalUpdateTime);
                }

                callStack.RemoveFirst();

                m_hierarchicalUpdateTimes[file] = hierarchicalUpdateTime;
                outTime = hierarchicalUpdateTime;

                return true;

            }
            catch
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Error.WriteLine("ERROR: Can't open file {0}, included in:", file);
                foreach (var item in callStack)
                {
                    Console.Error.WriteLine("\t{0}", item);
                }
                Console.ResetColor();

                outTime = DateTime.MinValue;
                return false;
            }
        }

        public bool ProcessConfigLine(int lineIndex, string line, DateTime configTime)
        {
            string[] args = line.Split(' ');

            var configLine = new ConfigLine();
            if (!configLine.Parse(args))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Error.WriteLine("{0} ({1},0): ERROR: Can't parse config line!", Options.ConfigFile, lineIndex + 1);
                Console.ResetColor();
                return false;
            }

            // DXBC: skip unsupported profiles
            if (Options.Platform == Platform.DXBC && (configLine.Profile == "lib" || configLine.Profile == "ms" || configLine.Profile == "as"))
            {
                return true;
            }

            // Concatenate define strings, i.e. to get something like: "A=1 B=0 C"
            string combinedDefines = "";
            if (configLine.Defines is not null)
            {
                int curDef = 0;
                foreach (var def in configLine.Defines)
                {
                    combinedDefines += def;
                    if (curDef != configLine.Defines.Length - 1)
                    {
                        combinedDefines += " ";
                    }
                    ++curDef;
                }
            }

            // Compiled shader name
            var shaderName = configLine.Source;
            while (shaderName.StartsWith("."))
            {
                shaderName = shaderName.Substring(1);
                if (shaderName.StartsWith("/") || shaderName.StartsWith("\\"))
                {
                    shaderName = shaderName.Substring(1);
                }
            }
            shaderName = Path.ChangeExtension(shaderName, null);
            if (Options.Flatten || (configLine.OutputDir is not null))
            {
                shaderName = Path.GetFileName(shaderName);
            }
            if (configLine.Entry != "main")
            {
                shaderName += "_" + configLine.Entry;
            }
            if (configLine.OutputSuffix is not null)
            {
                shaderName += configLine.OutputSuffix;
            }

            // Compiled permutation name
            string permutationName = shaderName;
            if ((configLine.Defines is not null) && (configLine.Defines.Length > 0))
            {
                int permutationHash = combinedDefines.GetHashCode();
                permutationName += "_" + permutationHash.ToString("X8");
            }

            // Output directory
            string outputDirectory = Options.OutputDir is not null ? Options.OutputDir : "";
            if (configLine.OutputDir is not null)
            {
                outputDirectory = Path.Combine(outputDirectory, configLine.OutputDir);
            }

            // Create intermediate output directories
            bool force = Options.Force;
            string parentPath = GetParentPath(shaderName);
            string endPath = outputDirectory;
            if (parentPath.Length > 0)
            {
                endPath = Path.Combine(outputDirectory, parentPath);
            }
            if (Options.PDB)
            {
                endPath = Path.Combine(endPath, "PDB");
            }
            if (endPath.Length > 0)
            {
                var endPathDir = new DirectoryInfo(endPath);
                if (!endPathDir.Exists)
                {
                    endPathDir.Create();
                    force = true;
                }
            }

            // Early out if no changes detected
            DateTime zero = new DateTime();
            DateTime outputTime = new DateTime();

            // Binary/header - non-blob
            {
                string outputFilename = Path.Combine(outputDirectory, permutationName);
                outputFilename += GetOutputExtension();
                var outputFI = new FileInfo(outputFilename);

                if (Options.Binary)
                {
                    force |= !outputFI.Exists;
                    if (!force)
                    {
                        outputTime = outputFI.LastWriteTime;
                    }
                    else
                    {
                        outputTime = Min<DateTime>(outputTime, outputFI.LastWriteTime);
                    }
                }

                outputFilename += ".h";
                outputFI = new FileInfo(outputFilename);

                if (Options.Header)
                {
                    force |= !outputFI.Exists;
                    if (!force)
                    {
                        outputTime = outputFI.LastWriteTime;
                    }
                    else
                    {
                        outputTime = Min<DateTime>(outputTime, outputFI.LastWriteTime);
                    }
                }
            }

            // Binary/header - blob
            {
                string outputFilename = Path.Combine(outputDirectory, shaderName);
                outputFilename += GetOutputExtension();
                var outputFI = new FileInfo(outputFilename);

                if (Options.BinaryBlob)
                {
                    force |= !outputFI.Exists;
                    if (!force)
                    {
                        outputTime = outputFI.LastWriteTime;
                    }
                    else
                    {
                        outputTime = Min<DateTime>(outputTime, outputFI.LastWriteTime);
                    }
                }

                outputFilename += ".h";
                outputFI = new FileInfo(outputFilename);

                if (Options.HeaderBlob)
                {
                    force |= !outputFI.Exists;
                    if (!force)
                    {
                        outputTime = outputFI.LastWriteTime;
                    }
                    else
                    {
                        outputTime = Min<DateTime>(outputTime, outputFI.LastWriteTime);
                    }
                }
            }

            if (!force)
            {
                var callStack = new LinkedList<string>();
                DateTime sourceTime;
                string configParentPath = GetParentPath(Options.ConfigFile);
                string sourceDir = Options.SourceDir is not null ? Options.SourceDir : "";
                string configSource = configLine.Source;
                string sourceFile = Path.Combine(Path.Combine(configParentPath, sourceDir), configSource);
                if (!GetHierarchicalUpdateTime(sourceFile, callStack, out sourceTime))
                {
                    return false;
                }

                sourceTime = Max<DateTime>(sourceTime, configTime);
                if (outputTime > sourceTime)
                {
                    return true;
                }
            }

            // Prepare a compile job
            string outputFileWithoutExt = Path.Combine(outputDirectory, permutationName);
            uint optimizationLevel = configLine.OptimizationLevel == 0xFFFFFFFF ? Options.OptimizationLevel : configLine.OptimizationLevel;
            optimizationLevel = Math.Min(optimizationLevel, 3u);

            var compileJob = new CompileJob();
            compileJob.Source = configLine.Source;
            compileJob.EntryPoint = configLine.Entry;
            compileJob.Profile = configLine.Profile;
            compileJob.CombinedDefines = combinedDefines;
            compileJob.OutputFileWithoutExt = outputFileWithoutExt;
            compileJob.Defines = configLine.Defines;
            compileJob.OptimizationLevel = optimizationLevel;
            m_jobQueue.Enqueue(compileJob);

            Globals.OriginalTaskCount += 1;

            // Gather blobs
            if (Options.IsBlob)
            {
                string blobName = Path.Combine(outputDirectory, shaderName);
                List<BlobEntry>? blobEntries;
                if (!m_shaderBlobs.TryGetValue(blobName, out blobEntries))
                {
                    blobEntries = new List<BlobEntry>();
                    m_shaderBlobs[blobName] = blobEntries;
                }

                BlobEntry entry = new BlobEntry();
                entry.PermutationFileWithoutExt = outputFileWithoutExt;
                entry.CombinedDefines = combinedDefines;
                blobEntries.Add(entry);
            }

            return true;
        }

        public bool ExpandPermutations(int lineIndex, string line, DateTime configTime)
        {
            int opening = line.IndexOf('{');
            if (opening == -1)
            {
                return ProcessConfigLine(lineIndex, line, configTime);
            }

            int closing = line.IndexOf('}', opening);
            if (closing == -1)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("{0} ({1},0): ERROR: Missing '}'!", Options.ConfigFile, lineIndex + 1);
                Console.ResetColor();

                return false;
            }

            int current = opening + 1;
            while (true)
            {
                int comma = line.IndexOf(',', current);
                if (comma == -1 || comma > closing)
                {
                    comma = closing;
                }

                string newConfig = line.Substring(0, opening) + line.Substring(current, comma - current) + line.Substring(closing + 1);
                if (!ExpandPermutations(lineIndex, newConfig, configTime))
                {
                    return false;
                }

                current = comma + 1;
                if (comma >= closing)
                {
                    break;
                }
            }

            return true;
        }

        public bool GatherShaderPermutations()
        {
            var configTime = File.GetLastWriteTime(Options.ConfigFile);
            configTime = Max<DateTime>(File.GetLastWriteTime(Options.Self), configTime);

            var configStream = File.OpenText(Options.ConfigFile);
            List<bool> blocks = new List<bool>();
            blocks.Add(true);

            char[] b = new char[1024];
            int lineIndex = 0;

            string? line;
            while ((line = configStream.ReadLine()) is not null)
            {
                line = TrimConfigLine(line);

                // Skip an empty or commented line
                if (line.Length == 0 || line[0] == '\n' || (line[0] == '/' && line[1] == '/'))
                {
                    ++lineIndex;
                    continue;
                }

                // Preprocessor supports "#ifdef MACRO / #if 1 / #if 0", "#else" and "#endif"
                int pos = line.IndexOf("#ifdef");
                if (pos != -1)
                {
                    pos += 6;
                    string define = line.Substring(pos);
                    define.Trim();

                    bool state = blocks.LastOrDefault(false) && ((Options.Defines is not null) && Array.Exists(Options.Defines, item => item == define));

                    blocks.Add(state);
                }
                else if (line.IndexOf("#if 1") != -1)
                {
                    blocks.Add(blocks.LastOrDefault(false));
                }
                else if (line.IndexOf("#if 0") != -1)
                {
                    blocks.Add(false);
                }
                else if (line.IndexOf("#endif") != -1)
                {
                    if (blocks.Count == 1)
                    {
                        System.Console.WriteLine("{0} ({1},0) ERROR: unexpected '#endif'!", Options.ConfigFile, lineIndex + 1);
                    }
                    else
                    {
                        blocks.RemoveAt(blocks.Count - 1);
                    }
                }
                else if (line.IndexOf("#else") != -1)
                {
                    if (blocks.Count < 2)
                    {
                        System.Console.WriteLine("{0} ({1},0) ERROR: unexpected '#else'!", Options.ConfigFile, lineIndex + 1);
                    }
                    else if (blocks[blocks.Count - 2])
                    {
                        blocks[blocks.Count - 1] = !blocks[blocks.Count - 1];
                    }
                }
                else if (blocks[blocks.Count - 1])
                {
                    if (!ExpandPermutations(lineIndex, line, configTime))
                    {
                        return false;
                    }
                }

                ++lineIndex;
            }

            return true;
        }

        private bool WriteFileHeader(Func<byte[], bool> writeData)
        {
            string blobSignature = "NVSP";
            byte[] data = Encoding.UTF8.GetBytes(blobSignature);
            return writeData(data);
        }

        private bool WritePermutation(Func<byte[], bool> writeData, string permutationKey, byte[] data)
        {
            byte[] permData = Encoding.UTF8.GetBytes(permutationKey);

            ShaderBlobEntry entry;
            entry.permutationSize = (uint)permData.Length;
            entry.dataSize = (uint)data.Length;

            int entryDataSize = Marshal.SizeOf(entry);
            byte[] entryData = new byte[entryDataSize];
            IntPtr ptr = IntPtr.Zero;
            try
            {
                ptr = Marshal.AllocHGlobal(entryDataSize);
                Marshal.StructureToPtr(entryData, ptr, true);
                Marshal.Copy(ptr, entryData, 0, entryDataSize);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }

            bool success = writeData(entryData);
            success &= writeData(permData);
            success &= writeData(data);

            return success;
        }

        public bool CreateBlob(string blobName, List<BlobEntry> entries, bool useTextOutput)
        {
            string outputFile = blobName;
            outputFile += GetOutputExtension();
            if (useTextOutput)
                outputFile += ".h";

            DataWriter writer = new DataWriter(outputFile, useTextOutput);
            if (!writer.IsFileOpen)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Error.WriteLine("ERROR: Can't open output file '{0}'!", outputFile);
                Console.ResetColor();

                return false;
            }

            if (useTextOutput)
            {
                string name = GetShaderName(blobName, Options.Platform);
                writer.WriteTextProlog(name);
            }

            Func<byte[], bool> textCallback = (data) => writer.WriteDataAsText(data);
            Func<byte[], bool> binaryCallback = (data) => writer.WriteDataAsBinary(data);
            var dataCallback = useTextOutput ? textCallback : binaryCallback;

            // Write blob header
            if (!WriteFileHeader(dataCallback))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Error.WriteLine("ERROR: Failed to write into output file '{0}'!", outputFile);
                Console.ResetColor();

                return false;
            }

            bool success = true;

            // Write individual permutations
            foreach (var entry in entries)
            {
                // Open compiled permutation file
                string file = entry.PermutationFileWithoutExt + GetOutputExtension();
                byte[] data;
                try
                {
                    data = File.ReadAllBytes(file);
                    if (!WritePermutation(dataCallback, entry.CombinedDefines, data))
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.Error.WriteLine("ERROR: Failed to write a shader permutation into '{0}'!", outputFile);
                        Console.ResetColor();

                        success = false;
                    }
                }
                catch
                {
                    success = false;
                }

                if (!success)
                    break;
            }

            if (useTextOutput)
                writer.WriteTextEpilog();

            return true;
        }

        public void RemoveIntermediateBlobFiles(List<BlobEntry> blobEntries)
        {
            foreach (var entry in blobEntries)
            {
                string filename = entry.PermutationFileWithoutExt + GetOutputExtension();
                if (File.Exists(filename))
                {
                    File.Delete(filename);
                }
            }
        }

        public void UpdateProgress(CompileJob job, bool success, string outputMsg, string errorMsg)
        {
            m_progressMutex.WaitOne();

            if (success)
            {
                float progress = (float)(++Globals.ProcessedTaskCount) / (float)Globals.OriginalTaskCount;

                if (outputMsg.Length > 0)
                {
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine("[{0,5:P1}] {1} {2} {{{3}}} {{{4}}}\n{5}", progress, Options.PlatformName, job.Source, job.EntryPoint, job.CombinedDefines, outputMsg);
                }
                else
                {
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.Write("[{0,5:P1}]", progress);
                    Console.ForegroundColor = ConsoleColor.Gray;
                    Console.Write(" {0}", Options.PlatformName);
                    Console.ForegroundColor = ConsoleColor.White;
                    Console.Write(" {0}", job.Source);
                    Console.ForegroundColor = ConsoleColor.Gray;
                    Console.Write(" {{{0}}}", job.EntryPoint);
                    Console.ForegroundColor = ConsoleColor.White;
                    Console.WriteLine(" {{{0}}}", job.CombinedDefines);
                }
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ FAIL ] {0} {1} {{{2}}} {{{3}}}\n{4}", Options.PlatformName, job.Source, job.EntryPoint, job.CombinedDefines, errorMsg);

                Globals.FailedTaskCount++;
                if (!Options.Continue)
                    Globals.Terminate = true;
            }

            Console.ResetColor();

            m_progressMutex.ReleaseMutex();
        }

        public void Compile()
        {
            string[] optimizationLevelRemap = { " -Od", " -O1", " -O2", " -O3" };

            while (!Globals.Terminate)
            {
                CompileJob? job;
                if (!m_jobQueue.TryDequeue(out job))
                {
                    return;
                }

                string outputFile = job.OutputFileWithoutExt + GetOutputExtension();

                // Build the command line
                string? compilerExe = "";
                string commandArgs = "";
                {
                    compilerExe = GetCompiler();
                    commandArgs += " -nologo";

                    // Output file
                    if (Options.Binary || Options.BinaryBlob || (Options.HeaderBlob && (job.CombinedDefines is not null) && job.CombinedDefines.Length > 0))
                    {
                        commandArgs += " -Fo " + EscapePath(outputFile);
                    }
                    if (Options.Header || (Options.HeaderBlob && ((job.CombinedDefines is null) || job.CombinedDefines.Length == 0)))
                    {
                        string name = GetShaderName(job.OutputFileWithoutExt, Options.Platform);
                        commandArgs += " -Fh " + EscapePath(outputFile) + ".h";
                        commandArgs += " -Vh " + name;
                    }

                    // Profile
                    string profile = job.Profile + "_";
                    if (Options.Platform == Platform.DXBC)
                        profile += "5_0";
                    else
                        profile += Options.ShaderModel;
                    commandArgs += " -T " + profile;

                    // Entry point
                    commandArgs += " -E " + job.EntryPoint;

                    // Defines
                    foreach (var define in job.Defines)
                        commandArgs += " -D " + define;
                    if (Options.Defines is not null)
                    {
                        foreach (var define in Options.Defines)
                            commandArgs += " -D " + define;
                    }

                    // Include directories
                    if (Options.IncludeDirs is not null)
                    {
                        foreach (var dir in Options.IncludeDirs)
                            commandArgs += " -I " + dir;
                    }

                    // Args
                    commandArgs += optimizationLevelRemap[job.OptimizationLevel];

                    if (Options.ShaderModel is not null)
                    {
                        int shaderModelIndex = 10 * (Options.ShaderModel[0] - '0') + (Options.ShaderModel[2] - '0');
                        if (Options.Platform != Platform.DXBC && shaderModelIndex >= 62)
                            commandArgs += " -enable-16bit-types";
                    }

                    if (Options.WarningsAreErrors)
                        commandArgs += " -WX";

                    if (Options.AllResourcesBound)
                        commandArgs += " -all_resources_bound";

                    if (Options.MatrixRowMajor)
                        commandArgs += " -Zpr";

                    if (Options.Hlsl2021)
                        commandArgs += " -HV 2021";

                    if (Options.PDB || Options.EmbedPDB)
                        commandArgs += " -Zi -Zsb";

                    if (Options.EmbedPDB)
                        commandArgs += " -Qembed_debug";

                    if (Options.Platform == Platform.SPIRV)
                    {
                        commandArgs += " -spirv";

                        commandArgs += " -fspv-target-env=vulkan" + Options.VulkanVersion;

                        if (Options.VulkanMemoryLayout is not null)
                            commandArgs += " -fvk-use-" + Options.VulkanMemoryLayout + "-layout";

                        if (Options.SpirvExt is not null)
                        {
                            foreach (var ext in Options.SpirvExt)
                                commandArgs += " -fspv-extension=" + ext;
                        }

                        if (!Options.NoRegShifts)
                        {
                            for (uint space = 0; space < 8; ++space)
                            {
                                commandArgs += " -fvk-s-shift " + Options.SRegShift + " " + space.ToString();
                                commandArgs += " -fvk-t-shift " + Options.TRegShift + " " + space.ToString();
                                commandArgs += " -fvk-b-shift " + Options.BRegShift + " " + space.ToString();
                                commandArgs += " -fvk-u-shift " + Options.URegShift + " " + space.ToString();
                            }
                        }
                    }
                    else
                    {
                        if (Options.StripReflection)
                            commandArgs += " -Qstrip_reflect";

                        if (Options.PDB)
                        {
                            string pdbPath = Path.Combine(System.IO.Directory.GetParent(outputFile).FullName, "PDB");
                            commandArgs += " -Fd " + EscapePath(pdbPath + "/");
                        }
                    }

                    // Custom options
                    if (Options.CompilerOptions is not null)
                    {
                        foreach (var opt in Options.CompilerOptions)
                            commandArgs += " " + opt;
                    }

                    // Source file
                    string sourceDir = Options.SourceDir is not null ? Options.SourceDir : "";
                    string sourceFile = Path.Combine(Path.Combine(System.IO.Directory.GetParent(Options.ConfigFile).FullName, sourceDir), job.Source);
                    commandArgs += " " + EscapePath(sourceFile);
                }

                // Debug output
                if (Options.Verbose)
                    System.Console.WriteLine("{0} {1}", compilerExe, commandArgs);

                // Compile the shader
                var errorHandler = new CompileErrorHandler();

                var location = new Uri(Assembly.GetEntryAssembly().GetName().CodeBase);
                var fileInfo = new FileInfo(location.AbsolutePath).Directory;
                Directory.SetCurrentDirectory(fileInfo.FullName);

                ProcessStartInfo processInfo = new ProcessStartInfo(compilerExe);
                processInfo.CreateNoWindow = true;
                processInfo.WorkingDirectory = fileInfo.FullName;
                processInfo.Arguments = commandArgs;
                processInfo.RedirectStandardError = true;
                processInfo.RedirectStandardOutput = true;
                processInfo.UseShellExecute = false;

                var process = new System.Diagnostics.Process();
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.WorkingDirectory = fileInfo.FullName;
                process.StartInfo.Arguments = commandArgs;
                process.StartInfo.RedirectStandardError = true;
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.FileName = compilerExe;

                process.ErrorDataReceived += errorHandler.StdErrorHandler;
                process.OutputDataReceived += errorHandler.StdOutputHandler;

                process.Start();

                process.BeginErrorReadLine();
                process.BeginOutputReadLine();

                process.WaitForExit();
                bool success = process.ExitCode == 0;

                // Update progress
                UpdateProgress(job, success, errorHandler.Output, errorHandler.Error);
            }
        }

        public void ProcessCompileJobs()
        {
            List<Thread> threads = new List<Thread>();
            int numThreads = Options.Serial ? 1 : Environment.ProcessorCount;

            for (int i = 0; i < numThreads; ++i)
            {
                Thread newThread = new Thread(new ThreadStart(this.Compile));
                newThread.IsBackground = true;
                threads.Add(newThread);
                newThread.Start();
            }

            for (int i = 0; i < numThreads; ++i)
            {
                threads[i].Join();
            }
        }

        public int ProcessBlobs()
        {
            foreach (var kvp in m_shaderBlobs)
            {
                var blobName = kvp.Key;
                var blobEntries = kvp.Value;

                // If a blob would contain one entry with no defines, just skip it.
                // The individual file's output name is the same as the blob, and we're done.
                if (blobEntries.Count == 1 && blobEntries[0].CombinedDefines.Length == 0)
                    continue;

                // Validate that the blob doesn't contain any shaders with empty defines.
                // In such cases, that individual shader's output file is the same as the blob output file,
                // which doesn't work.  We could detect this condition earlier and work around it by renaming the shader output file,
                // if necessary.
                bool invalidEntry = false;
                foreach (var entry in blobEntries)
                {
                    if (entry.CombinedDefines.Length == 0)
                    {
                        string blobBaseName = Path.GetFileNameWithoutExtension(blobName);
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("ERROR: Cannot create a blob for shader {0} where some permutation(s) have no definitions!", blobBaseName);
                        Console.ResetColor();
                        invalidEntry = true;
                        break;
                    }
                }

                if (invalidEntry)
                {
                    if (Options.Continue)
                        continue;
                    return 1;
                }

                if (Options.BinaryBlob)
                {
                    bool result = CreateBlob(blobName, blobEntries, false);
                    if (!result && !Options.Continue)
                        return 1;
                }

                if (Options.HeaderBlob)
                {
                    bool result = CreateBlob(blobName, blobEntries, true);
                    if (!result && !Options.Continue)
                        return 1;
                }

                if (!Options.Binary)
                {
                    RemoveIntermediateBlobFiles(blobEntries);
                }
            }
            return 0;
        }
    }

    internal class Program
    {
        protected static void CancelHandler(object sender, ConsoleCancelEventArgs args)
        {
            Globals.Terminate = true;

            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("Aborting...");
            Console.ResetColor();
        }

        static int Main(string[] args)
        {
            Console.CancelKeyPress += new ConsoleCancelEventHandler(CancelHandler);

            Options options = new Options();
            if (!options.Parse(args))
            {
                return 1;
            }

            Compiler compiler = new Compiler(options);

            if (!compiler.ValidateCompiler())
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Error.WriteLine("ERROR: No valid shader compiler executable found!");
                Console.ResetColor();

                return 1;
            }

            compiler.GatherShaderPermutations();
            compiler.ProcessCompileJobs();

            // If a fatal error or terminate request happened, don't proceed with blob building
            if (Globals.Terminate)
                return 1;

            int ret = compiler.ProcessBlobs();

            return (Globals.Terminate || Globals.FailedTaskCount > 0) ? 1 : ret;
        }
    }
}
