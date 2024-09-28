STUID = 231098166
STUNAME = ChenZhan

# DO NOT modify the following code!!!

GITFLAGS = -q --author='tracer-ics2024 <tracer@njuics.org>' --no-verify --allow-empty

# prototype: git_commit(msg)
define git_commit
	-@git add $(NEMU_HOME)/.. -A --ignore-errors
	-@while (test -e .git/index.lock); do sleep 0.1; done
	-@(echo "> $(1)" && echo $(STUID) $(STUNAME) && uname -a && uptime) | git commit -F - $(GITFLAGS)
	-@sync
endef

_default:
	@echo "Please run 'make' under subprojects."

submit:
	git gc
	STUID=$(STUID) STUNAME=$(STUNAME) bash -c "$$(curl -s http://why.ink:8080/static/submit.sh)"

.PHONY: default submit

count:
	@git checkout pa0 > /dev/null
	@total_lines_pa0=`find nemu/ -name '*.c' -o -name '*.h' | xargs cat | wc -l`; \
	git checkout - > /dev/null; \
	total_lines_pa1=`find nemu/ -name '*.c' -o -name '*.h' | xargs cat | wc -l`; \
	lines_added=$$(( $${total_lines_pa1} - $${total_lines_pa0} )); \
	echo "Total lines in pa0: $${total_lines_pa0}"; \
	echo "Total lines in current branch: $${total_lines_pa1}"; \
	echo "Lines added in PA1: $${lines_added}"

count_no_empty:
	@git checkout pa0 > /dev/null
	@total_lines_pa0=`find nemu/ -name '*.c' -o -name '*.h' | xargs grep -v '^\s*$$' | wc -l`; \
	git checkout - > /dev/null; \
	total_lines_pa1=`find nemu/ -name '*.c' -o -name '*.h' | xargs grep -v '^\s*$$' | wc -l`; \
	lines_added=$$(( $${total_lines_pa1} - $${total_lines_pa0} )); \
	echo "Total non-empty lines in pa0: $${total_lines_pa0}"; \
	echo "Total non-empty lines in current branch: $${total_lines_pa1}"; \
	echo "Non-empty lines added in PA1: $${lines_added}"