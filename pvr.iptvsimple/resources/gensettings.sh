#!/bin/sh
DIR="$(dirname "$0")"
OUTPUT="$DIR/settings.xml"
POS_SHIFT='-1'
NUM_SOURCES='10'

HEADER='<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<settings>
  <category label="30010">
    <!-- Channels list -->
    <setting label="30053" type="lsep"/> 
    <setting id="startNum" type="number" label="30013" default="1"/>
    <setting id="groupNameFormat_0" type="text" label="30054" default="%s"/>
    <setting id="channelNameFormat_0" type="text" label="30055" default="%s"/>
    
    <!-- EPG -->
    <setting label="30020" type="lsep"/>
    <setting id="epgPathType_0" type="enum" label="30000" lvalues="30001|30002" default="1"/>
    <setting id="epgPath_0" type="file" label="30021" default="" visible="eq(-1,0)"/>
    <setting id="epgUrl_0" type="text" label="30022" default="" visible="eq(-2,1)"/>
    <setting id="epgCache_0" type="bool" label="30026" default="true" visible="eq(-3,1)"/>
    <setting id="epgTimeShift_0" type="slider" label="30024" default="0" range="-12,.5,12" option="float"/>
    <setting id="epgTSOverride_0" type="bool" label="30023" default="false"/>
    
    <!-- Logos -->
    <setting label="30030" type="lsep"/>
    <setting id="logoPathType_0" type="enum" label="30000" lvalues="30001|30002" default="1"/>
    <setting id="logoPath_0" type="folder" label="30031" default="" visible="eq(-1,0)"/>
    <setting id="logoBaseUrl_0" type="text" label="30032" default="" visible="eq(-2,1)"/>
    <setting id="logoFileNameFormat_0" type="text" label="30056" default="%s"/>
  </category>
  
  <category label="30050">
    <setting id="sourceCount" type="enum" label="30051" default="0" values="@SOURCES@"/>'
SOURCE_SETTINGS='
    <!-- _______________________________ Source @SRC@ _____________________________________ -->
    <setting label="" type="lsep" visible="gt(@POS@,@COUNT@)"/>
    <setting label="$ADDON[pvr.iptvsimple 30052] @SRC@" type="lsep" visible="gt(@POS@,@COUNT@)"/>
    <setting type="sep" visible="gt(@POS@,@COUNT@)"/>

    <!-- Channels list -->
    <setting label="30053" type="lsep" visible="gt(@POS@,@COUNT@)"/> 
    <setting id="m3uPathType_@SRC@" type="enum" label="30000" lvalues="30001|30002" default="1" visible="gt(@POS@,@COUNT@)"/>
    <setting id="m3uPath_@SRC@" type="file" label="30011" default="" visible="eq(-1,0)+gt(@POS@,@COUNT@)"/>
    <setting id="m3uUrl_@SRC@" type="text" label="30012" default="" visible="eq(-2,1)+gt(@POS@,@COUNT@)"/>
    <setting id="m3uCache_@SRC@" type="bool" label="30025" default="true" visible="eq(-3,1)+gt(@POS@,@COUNT@)"/>
    <setting id="groupNameFormat_@SRC@" type="text" label="30054" default="%s" visible="gt(@POS@,@COUNT@)"/>
    <setting id="channelNameFormat_@SRC@" type="text" label="30055" default="%s" visible="gt(@POS@,@COUNT@)"/>
    
    <!-- EPG -->
    <setting label="30020" type="lsep" visible="gt(@POS@,@COUNT@)"/>
    <setting id="epgPathType_@SRC@" type="enum" label="30000" lvalues="30001|30002" default="1" visible="gt(@POS@,@COUNT@)"/>
    <setting id="epgPath_@SRC@" type="file" label="30021" default="" visible="eq(-1,0)+gt(@POS@,@COUNT@)"/>
    <setting id="epgUrl_@SRC@" type="text" label="30022" default="" visible="eq(-2,1)+gt(@POS@,@COUNT@)"/>
    <setting id="epgCache_@SRC@" type="bool" label="30026" default="true" visible="eq(-3,1)+gt(@POS@,@COUNT@)"/>
    <setting id="epgTimeShift_@SRC@" type="slider" label="30024" default="0" range="-12,.5,12" option="float" visible="gt(@POS@,@COUNT@)"/>
    <setting id="epgTSOverride_@SRC@" type="bool" label="30023" default="false" visible="gt(@POS@,@COUNT@)"/>
    
    <!-- Logos -->
    <setting label="30030" type="lsep" visible="gt(@POS@,@COUNT@)"/>
    <setting id="logoPathType_@SRC@" type="enum" label="30000" lvalues="30001|30002" default="1" visible="gt(@POS@,@COUNT@)"/>
    <setting id="logoPath_@SRC@" type="folder" label="30031" default="" visible="eq(-1,0)+gt(@POS@,@COUNT@)"/>
    <setting id="logoBaseUrl_@SRC@" type="text" label="30032" default="" visible="eq(-2,1)+gt(@POS@,@COUNT@)"/>
    <setting id="logoFileNameFormat_@SRC@" type="text" label="30056" default="%s" visible="gt(@POS@,@COUNT@)"/>
    <setting id="logoFromEpg_@SRC@" type="enum" label="30041" default="0" lvalues="30042|30043|30044" visible="gt(@POS@,@COUNT@)"/>

    <setting type="sep" visible="gt(@POS@,@COUNT@)"/>
    <!-- _____________________________________________________________________________ -->'
FOOTER='
  </category>
</settings>'

gen_source_settings() {
    local src_num="$1"
    local count="$(($src_num - 2))"
    local pos_shift="$POS_SHIFT"
    
    echo "$SOURCE_SETTINGS" | while read line
    do
        echo "    $line" | sed "s/@SRC@/$src_num/g; s/@POS@/$pos_shift/g; s/@COUNT@/$count/g"
        echo "$line" | grep -q '<setting ' && pos_shift="$((pos_shift - 1))"
    done
}

gen_settings() {
    echo "$HEADER" | sed "s/@SOURCES@/$(seq -s '|' 1 $NUM_SOURCES)/"
    
    for i in $(seq 1 $NUM_SOURCES)
    do
        gen_source_settings $i
        POS_SHIFT="$((POS_SHIFT - $(echo "$SOURCE_SETTINGS" | grep '<setting ' | wc -l)))"
    done
    
    echo "$FOOTER"
}

gen_settings | tee "$OUTPUT"
