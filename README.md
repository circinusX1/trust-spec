### Extend HAB frmom u-boot and kernel to OS using a simple kernel driver
#### Prerequisites
 * u-boot is signed with HAB
 * kernel too

-- --
####  How it works
 * a build in kernel driver. Not as module. (or signed module)
 * First bootup, finds a private key, check it against comopiled in public key /dev/mco-sane and signs the user space utility 'sanesystem' and 'sanesystem.conf'
 * Kernel driver runs sanesystem CREATE to create a permanent system hash file based on files sizes, timetamp and CRC or commands output, see conf file
    * sanesystem then signs using private key the permanent system hash
    * sanesystem deletes the private key
 * Next bootups
 * Kernel driver checks sanesystem against it's signature. If any file was changed it yields error. TODO, kill system.
 * Then runs sanesystem VERIFY application
 * The sanesystem then checks first if the hash has been alterred against it's the signature file.  If any file was changed it yields error. TODO, kill system.
 * Runs the check again and compare the original hash with current sysstem scan. 

-- --




<img width="986" height="767" alt="image" src="https://github.com/user-attachments/assets/1a89e2d7-e2c8-46c0-a353-c9c25bc42bd7" />


