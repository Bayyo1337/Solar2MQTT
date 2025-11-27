Import("env")

env.Append(CPPDEFINES=[
    ("SWVERSION", env.StringifyMacro(env.GetProjectOption("custom_prog_version"))),
    ("HWBOARD", env.StringifyMacro(env["PIOENV"])),
])

# Use GetProjectOption with a default value to avoid errors if the option is missing
custom_hardwareserial = env.GetProjectOption("custom_hardwareserial", default="false")

if custom_hardwareserial == "true":
    env.Append(CPPDEFINES=[
        ("isUART_HARDWARE",  env.StringifyMacro(env.GetBuildType())),
    ])

env.Replace(PROGNAME="Solar2MQTT_%s_%s" % (str(env["PIOENV"]), env.GetProjectOption("custom_prog_version")))
