pacman-key --recv F7FD5492264BB9D0
pacman-key --lsign F7FD5492264BB9D0
echo [dkp-libs] >> /etc/pacman.conf
echo Server = http://downloads.devkitpro.org/packages >> /etc/pacman.conf
echo [dkp-windows] >> /etc/pacman.conf
echo Server = http://downloads.devkitpro.org/packages/windows >> /etc/pacman.conf
pacman --noconfirm -U https://downloads.devkitpro.org/devkitpro-keyring-r1.787e015-2-any.pkg.tar.xz
pacman --noconfirm -Syu
