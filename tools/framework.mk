
# $(if ${verbose},,@echo ' ' $1 $@;)
define F_QUIET_TOOL
$(if ${verbose},,@printf '  %-7s %s\n' $1 $@;)
endef

define F_QUIET_TOOL_2
$(if ${verbose},,@printf '  %-7s %s\n' $1 $2;)
endef

# Utility variables reducing output verbosity. Place these variables in front of
# calls to their respective tools (e.g. quiet_cc in front of CC) and when
# "verbose" is not set then any calls to CC display "CC <target>" instead of the
# full command string. Set "verbose" to any value to display the full command.
verbose       ?=
# verbose       := true
quiet_as      = $(call F_QUIET_TOOL,"AS")
quiet_cpp     = $(call F_QUIET_TOOL,"CPP")
quiet_cc      = $(call F_QUIET_TOOL,"CC")
quiet_cxx     = $(call F_QUIET_TOOL,"CXX")
quiet_ar      = $(call F_QUIET_TOOL,"AR")
quiet_ld      = $(call F_QUIET_TOOL,"LD")
quiet_objcopy = $(call F_QUIET_TOOL,"OBJCOPY")
quiet_objdump = $(call F_QUIET_TOOL,"OBJDUMP")
quiet_size    = $(call F_QUIET_TOOL,"SIZE")
quiet_mkdir   = $(call F_QUIET_TOOL,"MKDIR")

# ------------------------------------------------------------------------------

# @brief      Convert provided path(s) to canonical form
#
#             A canonical path is one without "./" or "../" and is relative to
#             ${CURDIR}; if a provided path is absolute then the canonical form
#             is the absolute path.
#
# @param      ${1}  Path(s) to convert to canonical form
#
# @note       ${CURDIR} = Pathname of the current working directory (after all
#             -C options) $(abspath names...) For each name returns an absolute
#             path name that does not contain any . or .. components, nor any
#             repeated path separators (/). Does not resolve symbolic links and
#             does not require the file names to refer to an existing file or
#             directory.
define F_CANONICAL_PATH
$(patsubst ${CURDIR}/%,%,$(abspath ${1}))
endef

# @brief      Prepend @p ${1} to provided non-absolute path(s)
#
#             This function handles both Unix (/) and Windows ([A-Z]:/) absolute
#             paths. To handle Windows absolute paths it searches for a colon
#             ":" in the pathname.
#
# @param      ${1}  Prefix to prepend to a path
# @param      ${2}  Path(s) to operate on
define F_QUALIFY_PATH
 $(addprefix ${1}/,$(filter-out /%,${2})) $(filter /%,${2})
endef
 # $(foreach p,${2},$(if $(or $(findstring :,${p}),$(filter /%,${p})),${p},$(addprefix ${1}/,${p})))

# ------------------------------------------------------------------------------

# Automatic dependency generation configuration
# 
# See the link provided below for an explanation of DEPFLAGS and POSTCOMPILE.
#
# Based on:
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
DEPFLAGS       = -MT $@ -MMD -MP -MF $(basename $@).Td
deppostcompile = @mv -f $(basename $@).Td $(basename $@).d && touch $@

# ------------------------------------------------------------------------------

# Space-separated list of file extensions for respective compilers (CC and CXX);
# case-sensitive. This may be overridden before including scaffolding.mk.
cc_src_exts  ?= %.c
cxx_src_exts ?= %.cpp %.cxx %.cc

# @brief      Object rule template
#
#             When used with eval, this rule will create an object rule for a
#             target. This rule is intended to be used in conjunction with
#             "foreach" to create an object rule per source extension (e.g.
#             "cxx_src_exts = %.cxx %.cpp %.cc" would need three object rules),
#             all using the same build recipe (e.g. ET_CXX_RECIPE).
#
#             This template calls the recipe template, passing it the name of
#             the target (@a ${1}). This template *does not* eval it internally -
#             the eval performed on this template will also effect the recipe
#             template. Therefore,  @a ${4} should be the *name* of the recipe
#             template, not a variable reference to the template (i.e.
#             "ET_CXX_RECIPE" not "${ET_CXX_RECIPE}").
#             
# @note       Path portion of object pattern must match path of source file
#             pattern. For example, if the target is "build/%.o", the dependency
#             is "%.c", and the source file is "src/module/example.c" then the
#             object file must reside at "build/src/module/example.o" - this
#             means the intermediate path, "src/module", must be created (this
#             is handled by the target "depdirs" variable).
#
# @note       Must use with "eval"
#
# @param      ${1}  The name of the target
# @param      ${2}  Object directory path
# @param      ${3}  Source file pattern (e.g. %.c)
# @param      ${4}  Name of recipe template used to compile source file in
#                   object file
define ET_TGT_OBJ_RULE
    ${2}/${1}/%.o : ${3}
		$(call ${4},${1})
endef

# @param      ${1}  The name of the target
# @param      ${2}  The name of the target rule
define ET_TEST_TARGET_RULE
    dir := $$(call F_CANONICAL_PATH,$$(dir ${1}))
	
	# Initialize variables that may not have been set
    ${1}.incdirs  ?=
    ${1}.cppflags ?=
    ${1}.cflags   ?=
    ${1}.cxxflags ?=
    ${1}.ldflags  ?=
    ${1}.ldlibs   ?=

    # Convert list of include directories to include commands to compiler
    ${1}.incdirs := $$(call F_QUALIFY_PATH,$${dir},$${${1}.incdirs})
    ${1}.incdirs := $$(call F_CANONICAL_PATH,$${${1}.incdirs})
    ${1}.incdirs := $$(addprefix -I,$${${1}.incdirs})

    # Clean up sources list and convert to objects
    SRCS      := $$(call F_QUALIFY_PATH,$${dir},$${${1}.srcs})
    SRCS      := $$(call F_CANONICAL_PATH, $${SRCS})
    ${1}.objs := $$(addprefix $${objdir}/$${tgt}/,$$(addsuffix .o,$$(basename $${SRCS})))

    # Add order-only directory dependencies; "sort" is used to remove duplicates
    ${1}.depdirs += $${test.tgtdir} $${builddir} $${objdir}
    ${1}.depdirs += $$(call F_CANONICAL_PATH,$$(dir $${${1}.objs}))
    ${1}.depdirs := $$(sort $${${1}.depdirs})
    $$(foreach d,$${${1}.depdirs},\
        $$(eval $$(filter $${d}/%.o,$${${1}.objs}): | $${d}))
    depdirs += $${${1}.depdirs}

    # Add the target rule
    $$(eval $$(call ${2},${1}))

    # Add clean rule
    clean: clean_${1}
    .PHONY: clean_${1}
    clean_${1}:
		$${RM} $${test.tgtdir}/${1} $${${1}.objs:%.o=%.[od]}

   # Add help rule
    debug: debug_${1}
    .PHONY: debug_${1}
    debug_${1}:
		@echo '-- ${1} --';\
		 echo 'target   '[${test.tgtdir}/${1}];\
		 echo 'sources  '[$$(strip $${${1}.srcs})];\
		 echo 'objects  '[$$(strip $${${1}.objs})];\
		 echo 'cppflags '[$$(strip $${${1}.cppflags})];\
		 echo 'incdirs  '[$$(strip $${${1}.incdirs})];\
		 echo 'cflags   '[$$(strip $${${1}.cflags})];\
		 echo 'cxxflags '[$$(strip $${${1}.cxxflags})];\
		 echo 'ldflags  '[$$(strip $${${1}.ldflags})];\
		 echo 'ldlibs   '[$$(strip $${${1}.ldlibs})];\
		 echo
endef


# @param      ${1}  The name of the target
define ET_TEST_CC_RECIPE
	$${quiet_cxx}$$(strip $${test_CC} $${DEPFLAGS} $${${1}.cppflags} \
		$${test_cppflags} $${${1}.cflags} $${test_cflags} $${${1}.incdirs} \
		-c $$< -o $$@)
	$${deppostcompile}
endef

# @param      ${1}  The name of the target
define ET_TEST_CXX_RECIPE
	$${quiet_cxx}$$(strip $${test_CXX} $${DEPFLAGS} $${${1}.cppflags} \
		$${test_cppflags} $${${1}.cxxflags} $${test_cxxflags} $${${1}.incdirs} \
		-c $$< -o $$@)
	$${deppostcompile}
endef

# @param      ${1}  The name of the target
define ET_TEST_EXE_TGT_RULE
    # Executable target rule
    $${test.tgtdir}/${1}: $${${1}.objs} | $${test.tgtdir}
		$${quiet_ld}$$(strip $${test_LD} $${${1}.ldflags} $${test_ldflags}\
			$${${1}.objs} -o $$@ $${${1}.ldlibs} $${test_ldlibs})
endef

# @brief      Dependent directory rule template
#
#             When used with eval this template creates a rule to generate
#             dependent directories provided in the space-separated list, @a
#             ${1}.
#
# @note       Must use with "eval"
#
# @param      ${1}  Space-separated list of directories
define ET_DEP_RULE
    $$(sort ${1}):
		$${quiet_mkdir}mkdir -p $$@
endef
