local log, lpeg, fs, Preprocessor =
    require "log",
    require "lpeg",
    require "fs",
    require "Preprocessor"

log.println("%s - %s %s %s",
    CLASSYC.VERSION,
    jit.os,
    jit.arch,
    CLASSYC.BUILD_TYPE)
log.println("   (%s, %s, %s)",
    jit.version,
    lpeg.version,
    CLASSYC.CC_VERSION)

if #io.cargs == 0 then
    log.println("> usage:\tclassyc _source_file_ [ _source_file_ ... ]")
else
    log.println("> stage:\tpreprocessing")
    for i, strInputFile in ipairs(io.cargs) do
        local strOutputFile =
            strInputFile .. ".pp"

        local pp = Preprocessor.New()
        pp:Preprocess(strInputFile, strOutputFile)

        log.println("> input:\t%s", strInputFile)
        log.println("> output:\t%s", strOutputFile)
    end
end
