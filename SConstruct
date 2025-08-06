# Copyright 2025 Eric Smith
# SPDX-License-Identifier: GPL-3.0-only

conf_file = 'crest.conf'
vars = Variables(conf_file, ARGUMENTS)
vars.AddVariables(EnumVariable('target',
                               help = 'execution target platform',
                               allowed_values = ('posix', 'win32', 'win64'),
                               default = 'posix',
                               ignorecase = True))
env = Environment(variables = vars)
vars.Save(conf_file, env)
Help(vars.GenerateHelpText(env))

platform = env['PLATFORM']
target = env['target']

build_dir =  '#build/' + target
env['build_dir'] = build_dir

env['CROSS'] = (target == platform)

env.Append(CPPPATH = [build_dir])  # for auto-generated sources or headers

#-----------------------------------------------------------------------------
# executables
#-----------------------------------------------------------------------------

executables = SConscript('src/SConscript',
                         duplicate = False,
                         variant_dir = build_dir,
                         exports = ['env'])

# Local Variables:
# mode: python
# End:
