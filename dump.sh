TARGET_DIR=/sdcard/LSP
TARGET=dump_$(date +%Y%m%d_%H%M%S).txt

su 0 setenforce 0

dumpsys battery
echo $?
sleep 1

if [ $# -ne 0 ]
then
    TARGET=$1
fi

date > $TARGET

echo "/d/wakeup_sources" >> $TARGET
cat /d/wakeup_sources >> $TARGET

echo "\n/sys/devices/system/cpu/cpu0/cpufreq/stats/time_in_state" >> $TARGET
cat /sys/devices/system/cpu/cpu0/cpufreq/stats/time_in_state >> $TARGET

echo "\n/proc/net/dev" >> $TARGET
cat /proc/net/dev >> $TARGET

echo "\n/proc/interrupts" >> $TARGET
cat /proc/interrupts >> $TARGET

echo "\n/proc/msm_pm_stats" >> $TARGET
cat /proc/msm_pm_stats >> $TARGET

echo "\npm list package" >> $TARGET
pm list package >> $TARGET

echo "\ndumpsys batterystats" >> $TARGET
dumpsys batterystats >> $TARGET

echo "\ndumpsys content" >> $TARGET
dumpsys content >> "$TARGET"

echo "\ndumpsys location" >> $TARGET
dumpsys location >> $TARGET

echo "\ndumpsys alarm" >> $TARGET
dumpsys alarm >> $TARGET

dumpsys batterystats --reset

echo "\ndumpsys notification" >> $TARGET
dumpsys notification >> $TARGET


mv $TARGET $TARGET_DIR/$TARGET

