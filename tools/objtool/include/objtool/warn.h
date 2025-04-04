/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2015 Josh Poimboeuf <jpoimboe@redhat.com>
 */

#ifndef _WARN_H
#define _WARN_H

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <objtool/builtin.h>
#include <objtool/elf.h>

extern const char *objname;

static inline char *offstr(struct section *sec, unsigned long offset)
{
	bool is_text = (sec->sh.sh_flags & SHF_EXECINSTR);
	struct symbol *sym = NULL;
	char *str;
	int len;

	if (is_text)
		sym = find_func_containing(sec, offset);
	if (!sym)
		sym = find_symbol_containing(sec, offset);

	if (sym) {
		str = malloc(strlen(sym->name) + strlen(sec->name) + 40);
		len = sprintf(str, "%s+0x%lx", sym->name, offset - sym->offset);
		if (opts.sec_address)
			sprintf(str+len, " (%s+0x%lx)", sec->name, offset);
	} else {
		str = malloc(strlen(sec->name) + 20);
		sprintf(str, "%s+0x%lx", sec->name, offset);
	}

	return str;
}

#define WARN(format, ...)				\
	fprintf(stderr,					\
		"%s: %s: objtool: " format "\n",	\
		objname,				\
		opts.werror ? "error" : "warning",	\
		##__VA_ARGS__)

#define WARN_FUNC(format, sec, offset, ...)		\
({							\
	char *_str = offstr(sec, offset);		\
	WARN("%s: " format, _str, ##__VA_ARGS__);	\
	free(_str);					\
})

#define WARN_LIMIT 2

#define WARN_INSN(insn, format, ...)					\
({									\
	struct instruction *_insn = (insn);				\
	BUILD_BUG_ON(WARN_LIMIT > 2);					\
	if (!_insn->sym || _insn->sym->warnings < WARN_LIMIT) {		\
		WARN_FUNC(format, _insn->sec, _insn->offset,		\
			  ##__VA_ARGS__);				\
		if (_insn->sym)						\
			_insn->sym->warnings++;				\
	} else if (_insn->sym && _insn->sym->warnings == WARN_LIMIT) {	\
		WARN_FUNC("skipping duplicate warning(s)",		\
			  _insn->sec, _insn->offset);			\
		_insn->sym->warnings++;					\
	}								\
})

#define BT_INSN(insn, format, ...)				\
({								\
	if (opts.verbose || opts.backtrace) {			\
		struct instruction *_insn = (insn);		\
		char *_str = offstr(_insn->sec, _insn->offset); \
		WARN("  %s: " format, _str, ##__VA_ARGS__);	\
		free(_str);					\
	}							\
})

#define WARN_ELF(format, ...)				\
	WARN(format ": %s", ##__VA_ARGS__, elf_errmsg(-1))

#endif /* _WARN_H */
