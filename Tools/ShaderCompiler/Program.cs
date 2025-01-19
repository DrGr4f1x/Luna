using Microsoft.Win32;
using System;
using System.Collections;
using System.Collections.Generic;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.CommandLine.IO;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.IO;
using System.Numerics;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace ShaderCompiler
{
    class Options
    {
        #region Required options
        public string? Platform { get; set; }
        public FileInfo? ConfigFile { get; set; }
        public DirectoryInfo? Out { get; set; }
        public bool Binary { get; set; } = false;
        public bool Header { get; set; } = false;
        public bool BinaryBlob { get; set; } = false;
        public bool HeaderBlob { get; set; } = false;
        #endregion

        #region Compiler settings
        public string? ShaderModel { get; set; } = "6_5";
        public int Optimization { get; set; } = 3;
        public bool WarningsAreErrors { get; set; } = false;
        public bool AllResourcesBound { get; set; } = false;
        public bool PDB { get; set; } = false;
        public bool EmbedPDB { get; set; } = false;
        public bool StripReflection { get; set; } = false;
        public bool MatrixRowMajor { get; set; } = false;
        public bool Hlsl2021 { get; set; } = false;
        public string? VulkanMemoryLayout { get; set; }
        public string[]? CompilerOptions { get; set; }
        #endregion

        #region Defines and includes
        public DirectoryInfo[]? IncludeDirs { get; set; }
        public string[]? Defines { get; set; }
        #endregion

        #region Other options
        public bool Force { get; set; } = false;
        public DirectoryInfo? SourceDir { get; set; }
        public FileInfo[]? RelaxedIncludes { get; set; }
        public string? OutputExt { get; set; }
        public bool Serial { get; set; } = false;
        public bool Flatten { get; set; } = false;
        public bool Continue { get; set; } = false;
        public bool Verbose { get; set; } = false;
        public int RetryCount { get; set; } = 10;
        #endregion

        #region SPIRV options
        public string? VulkanVersion { get; set; } = "1.3";
        public string[]? SpirvExt { get; set; }
        public int SRegShift { get; set; } = 100;
        public int TRegShift { get; set; } = 200;
        public int BRegShift { get; set; } = 300;
        public int URegShift { get; set; } = 400;
        public bool NoRegShifts { get; set; } = false;
        #endregion

        public bool Validate()
        {
            // Check that the config file exists
            if (ConfigFile != null && !ConfigFile.Exists)
            {
                System.Console.Error.WriteLine("ERROR: Config file {0} does not exist!", ConfigFile.FullName);
                return false;
            }

            // Create output directory if it does not exist
            if (Out != null && !Out.Exists)
            {
                try
                {
                    Out.Create();
                }
                catch (IOException ex)
                {
                    System.Console.Error.WriteLine("ERROR: Could not create output directory {0}!  Exception: {1}.", Out.FullName, ex.Message);
                    return false;
                }
            }

            // Check output type
            if (!Binary && !Header && !BinaryBlob && !HeaderBlob)
            {
                System.Console.Error.WriteLine("ERROR: One of 'binary', 'header', 'binaryBlob', or 'headerBlob' must be set!");
                return false;
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
        public List<string> Defines { get; set; }
        public string Source { get; set; }
        public string EntryPoint { get; set; }
        public string Profile { get; set; }
        public string OutputFileWithoutExt { get; set; }
        public string CombinedDefines { get; set; }
        public int OptimizationLevel { get; set; } = 3;
    }

    class ConfigLine
    {
        public string Source { get; set; } = "";
        public string[]? Defines { get; set; }
        public string? Entry { get; set; } = "main";
        public string? Profile { get; set; } = null;
        public string? OutputDir { get; set; } = null;
        public string? OutputSuffix { get; set; } = null;
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

    class Compiler
    {
        private FileInfo? m_vulkanDxcCompiler = null;
        private FileInfo? m_windowsDxcCompiler = null;
        private FileInfo? m_windowsFxcCompiler = null;

        public FileInfo Self { get; set; }
        public Options Options { get; set; }

        public Compiler(FileInfo self, Options options)
        {
            this.Self = self;
            this.Options = options;

            FindVulkanDxcCompiler();
            FindWindowsCompilers();
        }

        public void FindVulkanDxcCompiler()
        {
            string? vulkanSdkPath = System.Environment.GetEnvironmentVariable("VULKAN_SDK");
            if (vulkanSdkPath != null)
            {
                m_vulkanDxcCompiler = new FileInfo(vulkanSdkPath + "\\Bin\\dxc.exe");
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
                       
            if(installedRootsKey != null)
            {
                var kitsRootDirectory = new DirectoryInfo((string)installedRootsKey.GetValue("KitsRoot10"));
                
                if( kitsRootDirectory.Exists)
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
                            {
                                break;

                            }

                            var x64Dir = new DirectoryInfo(dir.FullName + "\\x64");
                            if (!x64Dir.Exists)
                            {
                                continue;
                            }

                            var files = x64Dir.GetFiles("*.exe");
                            foreach (var fi in files)
                            {
                                if (!bFoundDxc && fi.Name == "dxc.exe")
                                {
                                    m_windowsDxcCompiler = fi;
                                    bFoundDxc = true;
                                }

                                if (!bFoundFxc && fi.Name == "fxc.exe")
                                {
                                    m_windowsFxcCompiler = fi;
                                    bFoundFxc = true;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                System.Console.WriteLine("Could not find expected registry key {0}", installedRootsPath);
            }
        }

        public void PrintStatus()
        {
            if (m_vulkanDxcCompiler is not null && m_vulkanDxcCompiler.Exists)
            {
                System.Console.WriteLine("Vulkan SDK dxc compiler: {0}", m_vulkanDxcCompiler.FullName);
            }
            else
            {
                System.Console.WriteLine("Vulkan SDK dxc compiler not found!");
            }

            if (m_windowsDxcCompiler is not null && m_windowsDxcCompiler.Exists)
            {
                System.Console.WriteLine("Windows SDK dxc compiler: {0}", m_windowsDxcCompiler.FullName);
            }
            else
            {
                System.Console.WriteLine("Windows SDK dxc compiler not found!");
            }

            if (m_windowsFxcCompiler is not null && m_windowsFxcCompiler.Exists)
            {
                System.Console.WriteLine("Windows SDK fxc compiler: {0}", m_windowsFxcCompiler.FullName);
            }
            else
            {
                System.Console.WriteLine("Windows SDK fxc compiler not found!");
            }
        }

        public static T Max<T>(T first, T second)
        {
            if (Comparer<T>.Default.Compare(first, second) > 0)
                return first;
            return second;
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

        public bool ProcessConfigLine(int lineIndex, string line, DateTime configTime)
        {
            System.Console.WriteLine("ProcessConfigLine {0}: {1}", lineIndex, line);

            string[] args = line.Split(' ');

            var configLine = new ConfigLine();
            if (!configLine.Parse(args))
            {
                return false;
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
                System.Console.WriteLine("{0} ({1},0): ERROR: Missing '}'!", Options.ConfigFile.FullName, lineIndex + 1);
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
            var configTime = Options.ConfigFile.LastWriteTime;
            configTime = Max<DateTime>(Self.LastWriteTime, configTime);

            var configStream = Options.ConfigFile.OpenText();
            List<bool> blocks = new List<bool>();
            blocks.Add(true);

            char[] b = new char[1024];
            int lineIndex = 0;

            while (configStream.Peek() >= 0)
            {
                string line = configStream.ReadLine();
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

                    bool state = blocks.LastOrDefault(false) && Array.Exists(Options.Defines, item => item == define);

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
                        System.Console.WriteLine("{0} ({1},0) ERROR: unexpected '#endif'!", Options.ConfigFile.FullName, lineIndex + 1);
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
                        System.Console.WriteLine("{0} ({1},0) ERROR: unexpected '#else'!", Options.ConfigFile.FullName, lineIndex + 1);
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
    }

    internal class Program
    {
        static async Task<int> Run(FileInfo self, Options options)
        {
            if (!options.Validate())
            {
                return -1;
            }

            var compiler = new Compiler(self, options);
            compiler.PrintStatus();

            compiler.GatherShaderPermutations();

            return 0;
        }

        

        static async Task<int> Main(string[] args)
        {
            int returnCode = 0;

            #region Required options
            var platformOpt = new Option<string>("--platform", "DXIL or SPIRV").FromAmong("DXIL", "SPIRV");
            platformOpt.IsRequired = true;
            platformOpt.AddAlias("-p");

            var configFileOpt = new Option<FileInfo>("--config", "Configuration file with the list of shaders to compile");
            configFileOpt.IsRequired = true;
            configFileOpt.AddAlias("-c");

            var outputDirOpt = new Option<DirectoryInfo>("--out", "Output directory");
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

            var optimizationOpt = new Option<int>("--optimization", "Optimization level 03 (default = 3, disabled = 0)");
            optimizationOpt.AddAlias("-O");

            var warningsAreErrorsOpt = new Option<bool>("--WX", "Maps to '-WX' DXC/FXC option: warnings are errors");
            var allResourcesBoundOpt = new Option<bool>("--allResourcesBound", "MAps to -all_resources_bound DXC/FXC option: all resources bound");
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
            var includeOpt = new Option<DirectoryInfo[]>("--include", "Include directory(s)");
            includeOpt.AddAlias("-I");

            var defineOpt = new Option<string[]>("--define", "Macro definition(s) in forms 'M=value' or 'M'");
            defineOpt.AddAlias("-D");
            #endregion

            #region Other options
            var forceOpt = new Option<bool>("--force", "Treat all source files as modified");
            forceOpt.AddAlias("-f");

            var sourceDirOpt = new Option<DirectoryInfo>("--sourceDir", "Source code directory");
            var relaxedIncludeOpt = new Option<FileInfo[]>("--relaxedInclude", "Include file(s) not invoking re-compilation");
            var outputExtOpt = new Option<string>("--outputExt", "Extension for output files, default is one of .dxil or .spirv");
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

            var options = new Options();
            var self = new FileInfo(args[0]);

            rootCommand.SetHandler(async (context) =>
            {
                // Required options
                options.Platform = context.ParseResult.GetValueForOption(platformOpt);
                options.ConfigFile = context.ParseResult.GetValueForOption(configFileOpt);
                options.Out = context.ParseResult.GetValueForOption(outputDirOpt);
                options.Binary = context.ParseResult.GetValueForOption(binaryOpt);
                options.Header = context.ParseResult.GetValueForOption(headerOpt);
                options.BinaryBlob = context.ParseResult.GetValueForOption(binaryBlobOpt);
                options.HeaderBlob = context.ParseResult.GetValueForOption(headerBlobOpt);

                // Compiler options
                options.ShaderModel = context.ParseResult.GetValueForOption(shaderModelOpt);
                options.Optimization = context.ParseResult.GetValueForOption(optimizationOpt);
                options.WarningsAreErrors = context.ParseResult.GetValueForOption(warningsAreErrorsOpt);
                options.AllResourcesBound = context.ParseResult.GetValueForOption(allResourcesBoundOpt);
                options.PDB = context.ParseResult.GetValueForOption(pdbOpt);
                options.EmbedPDB = context.ParseResult.GetValueForOption(embedPDBOpt);
                options.StripReflection = context.ParseResult.GetValueForOption(stripReflectionOpt);
                options.MatrixRowMajor = context.ParseResult.GetValueForOption(matrixRowMajorOpt);
                options.Hlsl2021 = context.ParseResult.GetValueForOption(hlsl2021Opt);
                options.VulkanMemoryLayout = context.ParseResult.GetValueForOption(vulkanMemoryLayoutOpt);
                options.CompilerOptions = context.ParseResult.GetValueForOption(compilerOptionsOpt);

                // Defines and includes
                options.IncludeDirs = context.ParseResult.GetValueForOption(includeOpt);
                options.Defines = context.ParseResult.GetValueForOption(defineOpt);

                // Other options
                options.Force = context.ParseResult.GetValueForOption(forceOpt);
                options.SourceDir = context.ParseResult.GetValueForOption(sourceDirOpt);
                options.RelaxedIncludes = context.ParseResult.GetValueForOption(relaxedIncludeOpt);
                options.OutputExt = context.ParseResult.GetValueForOption(outputExtOpt);
                options.Serial = context.ParseResult.GetValueForOption(serialOpt);
                options.Flatten = context.ParseResult.GetValueForOption(flattenOpt);
                options.Continue = context.ParseResult.GetValueForOption(continueOnErrorOpt);
                options.Verbose = context.ParseResult.GetValueForOption(verboseOpt);
                options.RetryCount = context.ParseResult.GetValueForOption(retryCountOpt);

                // SPIRV options
                options.VulkanVersion = context.ParseResult.GetValueForOption(vulkanVersionOpt);
                options.SpirvExt = context.ParseResult.GetValueForOption(spirvExtOpt);
                options.SRegShift = context.ParseResult.GetValueForOption(sRegShiftOpt);
                options.TRegShift = context.ParseResult.GetValueForOption(tRegShiftOpt);
                options.BRegShift = context.ParseResult.GetValueForOption(bRegShiftOpt);
                options.URegShift = context.ParseResult.GetValueForOption(uRegShiftOpt);
                options.NoRegShifts = context.ParseResult.GetValueForOption(noRegShiftsOpt);

                returnCode = await Run(self, options);
            });

            await rootCommand.InvokeAsync(args);

            return returnCode;
        }
    }
}
