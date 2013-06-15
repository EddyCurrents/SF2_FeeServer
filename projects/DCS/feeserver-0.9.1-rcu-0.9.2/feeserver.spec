# -*- mode: rpm-spec -*- 
# $Id: feeserver.spec.in,v 1.1 2007/06/18 07:56:04 richter Exp $
%define debug_package 	  %{nil}
%define __find_provides   %{nil}

#_____________________________________________________________________
Summary: 	DCS card FeeServer
Name: 		feeserver
Version: 	0.9.1_rcu_0.9.2
Release: 	1
License: 	GNU General Public License
Group: 		System/Servers
URL: 		http://www.ift.uib.no/~kjeks/download
Source0: 	%{name}-0.9.1-rcu-0.9.2.tar.gz
BuildRoot: 	%{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildArch:	noarch
AutoReq:	0
Requires:	nfs-server

%description
This package contains the FeeServer for the DCS cards. Originally
developed for the RCU, the CE++ framework can also serve other
hardware.

The following functionality is anabled for this package:
 --enable-phos --enable-cxx-api=no

The server is installed in /home/richter/src/feeserver-0.9.1-rcu-0.9.2/build, which can then be served
via NFS to the DCS boards. On the DCS boards, the following should be 
setup

* The NFS directory /home/richter/src/feeserver-0.9.1-rcu-0.9.2/build should be mounted from host this package
  is installed on.   The directory should be exported with the following 
  options: 
	rw
	sync
	all_squash
	anonuid=99
	anongid=99

* One should create the symbolic link 
	`/dcs-share/boot' -> `/home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/etc/init.d'

#_____________________________________________________________________
%prep
%setup -q -n %{name}-0.9.1-rcu-0.9.2
./configure	\
	--prefix=/home/richter/src/feeserver-0.9.1-rcu-0.9.2/build \
	 --enable-phos --enable-cxx-api=no

%build
make 

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/var/log
mkdir -p $RPM_BUILD_ROOT/home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/var/run
export STRIP=/bin/true
export OBJDUMP=/bin/true

%clean
# rm -rf $RPM_BUILD_ROOT
make distclean > /dev/null 2>&1 

#_____________________________________________________________________
%files
%defattr(-,root,root,-)
%config(noreplace) /home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/etc/sysconfig/feeserver
%attr(775,nobody,nobody) /home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/var/log
%attr(775,nobody,nobody) /home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/var/run
/home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/lib
/home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/bin
/home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/etc
/home/richter/src/feeserver-0.9.1-rcu-0.9.2/build/include


#_____________________________________________________________________
%changelog
* Fri Jun  13 2007 Matthias Richter <Matthias.Richter@ift.uib.no> 
- added to project, 
- few generalizations.

* Sat Apr  7 2007 Christan Holm Christensen <cholm@nbi.dk> 
- Initial build.

