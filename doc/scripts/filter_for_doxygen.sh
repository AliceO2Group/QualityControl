#!/usr/bin/env bash
# Filtering out the HTML comments hiding doxygen keywords from Markdown

sed -e '/<!-- For Doxygen/d' -e '/---- end Doxygen -->/d' $1