#! /bin/sh

export mgwdir=`dirname $0`/gtk-win
. $mgwdir/config.sh || exit 1

islangsdir="$mgwprogramfilesdir/Inno Setup 5/Languages/"

mkdir /tmp/isfiles || exit 1
cd /tmp/isfiles

wget `echo '
http://www.jrsoftware.org/files/istrans/Catalan-4/Catalan-4-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Czech-5/Czech-5-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Danish-4/Danish-4-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Dutch-8/Dutch-8-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Finnish/Finnish-5.1.11.isl
http://www.jrsoftware.org/files/istrans/French-15/French-15-5.1.11.isl
http://www.jrsoftware.org/files/istrans/German-2/German-2-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Greek-8/Greek-8-5.5.0.isl
http://www.jrsoftware.org/files/istrans/Hebrew-3/Hebrew-3-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Hungarian/Hungarian-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Italian-14/Italian-14-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Japanese-5/Japanese-5-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Norwegian/Norwegian-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Polish-8/Polish-8-5.1.11.isl
http://www.jrsoftware.org/files/istrans/PortugueseBr-17/BrazilianPortuguese-17-5.5.0.isl
http://www.jrsoftware.org/files/istrans/PortugueseStd-1/PortugueseStd-1-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Russian-19/Russian-19-5.1.11.isl
http://www.jrsoftware.org/files/istrans/SerbianCyrillic-2/SerbianCyrillic-2-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Serbian-6/Serbian-6-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Slovenian-3/Slovenian-3-5.1.11.isl
http://www.jrsoftware.org/files/istrans/SpanishStd-5/SpanishStd-5-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Ukrainian-7/Ukrainian-7-5.1.11.isl
http://www.jrsoftware.org/files/istrans/Arabic-4/Arabic-4-5.5.0.isl
http://www.jrsoftware.org/files/istrans/Armenian-1/Armenian-1-5.5.0.islu
http://www.jrsoftware.org/files/istrans/Belarusian-2/Belarusian-2-5.5.0.isl
http://www.jrsoftware.org/files/istrans/Bosnian-2/Bosnian-2-5.5.0.isl
http://www.jrsoftware.org/files/istrans/Bulgarian/Bulgarian-5.5.0.isl
http://www.jrsoftware.org/files/istrans/ChineseSimp-12/ChineseSimp-12-5.5.0.isl
http://www.jrsoftware.org/files/istrans/Slovak-7/Slovak-7-5.5.0.isl
http://www.jrsoftware.org/files/istrans/Tatar-2/Tatar-2-5.5.0.isl
http://www.jrsoftware.org/files/istrans/Turkish-7/Turkish-7-5.5.0.isl
'`

islangs=/tmp/medit-islangs
echo "Name: \"english\"; MessagesFile: \"compiler:Default.isl\"" > $islangs
for file in *.isl *.islu; do
  lang=`echo $file | sed -r 's/(\w+)-.*\.(\w+)/\1/'`
  ext=`echo $file | sed -r 's/(\w+)-.*\.(\w+)/\2/'`
  echo mv $file $lang.$ext
  mv $file $lang.$ext || exit 1
  lang=`echo $lang | tr [A-Z] [a-z]`
  echo "Name: \"$lang\"; MessagesFile: \"compiler:Languages\\$lang.$ext\"" >> $islangs
done

echo mv *.isl *.islu "$islangsdir"
mv *.isl *.islu "$islangsdir" || exit 1
cd /tmp
rmdir isfiles
