### Extend HAB frmom u-boot and kernel to OS using a simple kernel driver
#### Prerequisites
 * u-boot is signed with HAB
 * kernel too

-- --
####  How it works
 * a build in kernel driver. Not as module. (or signed module)
 * First bootup, finds a private key, check it against kernel compiled-in public key /dev/mco-sane and signs the user space utility and i;s config 'sanesystem' and 'sanesystem.conf'
 * Kernel driver runs "sanesystem CREATE HASH privkey" to create a permanent system hash file based on files sizes, timetamp and CRC or commands output, see conf file.
    * sanesystem then signs using private key the permanent system hash
    * sanesystem deletes the private key
 * Next bootups
 * Kernel driver checks the user space prorgram sanesystem against it's signature. If any file was changed it yields error. TODO, kill system.
 * Then runs sanesystem VERIFY application
 * The sanesystem then checks first if the hash has been alterred against it's the signature file.  If any file was changed it yields error. TODO, kill system.
 * Runs the check again and compare the original hash with current sysstem scan.
 * If u-boot kernel kernel driver sanesystem, sanesystem.conf their signatures and hash and it;s sighature changed the framework prints and error.
 * If any of the files or fconfigurations frmo the config file changes as well the system yields an error.

##### The config file. 
```
threads=4
path=/etc,*.*,STC           # SizeTimeCrc
path=/usr/bin,*.*,ST
path=/usr/sbin,*.*,ST
path=/usr/lib,*.so,ST
path=/usr/lib,*.a,ST
path=/usr/lib/modules,*.ko,ST
ignore=/etc/mtab
command=fw_printenv,^mmcpart  # any uboot env except mmcpart has changed

```

-- --




<img width="981" height="988" alt="image" src="https://github.com/user-attachments/assets/4680efa1-3673-4c84-ba05-6c542241eee7" />



