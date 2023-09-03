#!/bin/sh

pandoc report.md \
	--toc=true \
	--toc-depth=5 \
	-V geometry:margin=3cm \
	-V author='Jack Kilrain (u6940136)' \
	-V date='03 September 2023' \
	-V title='Anubis UNIX Shell'\
	-N \
	-o u6940136_report.pdf
