#!/bin/bash
# patch translations
echo patch translations....
cd translations

# for NDC
git am ../icon-themes/ossii/0001-NDC-v2.0.2-translations.patch > /dev/null 2>&1|| git am --abort > /dev/null 2>&1

cd -
