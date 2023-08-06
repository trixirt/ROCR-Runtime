%global commit0 34545547518820ecf7d259455e09cb36f946f283
%global _name amd-rocm-runtime
%global rocm_path /opt/rocm
%global shortcommit0 %(c=%{commit0}; echo ${c:0:7})
%global up_name ROCR-Runtime

%define patch_level 2
%bcond_with debug
%bcond_with static

%if %{without debug}
%if %{without static}
Name:           %{_name}
%else
Name:           %{_name}-static
%endif
%else
%if %{without static}
Name:           %{_name}-debug
%else
Name:           %{_name}-static-debug
%endif
%endif

Version:        5.6
Release:        %{patch_level}.git%{?shortcommit0}%{?dist}
Summary:        TBD
License:        TBD

URL:            https://github.com/trixirt/%{up_name}
Source0:        %{url}/archive/%{commit0}/%{up_name}-%{shortcommit0}.tar.gz


BuildRequires:  clang
BuildRequires:  cmake
BuildRequires:  elfutils-libelf-devel
BuildRequires:  libdrm-devel
BuildRequires:  libffi-devel
BuildRequires:  lld
BuildRequires:  vim-common

%if %{without debug}
%global debug_package %{nil}
%endif
BuildRequires:  amd-hsakmt-devel
BuildRequires:  amd-rocm-device-libs-devel

%description
TBD

%package devel
Summary:        TBD

%description devel
%{summary}


%prep
%autosetup -p1 -n %{up_name}-%{commit0}


%build
%cmake -S src \
%if %{with static}
       -DBUILD_SHARED_LIBS=OFF \
%endif
%if %{without debug}
       -DCMAKE_BUILD_TYPE=RELEASE \
%else
       -DCMAKE_BUILD_TYPE=DEBUG \
%endif
       -DCMAKE_INSTALL_PREFIX=%{rocm_path}
%cmake_build

%install
%cmake_install

%files devel
%{rocm_path}

%changelog
* Sat Aug 05 2023 Tom Rix <trix@redhat.com>
- Stub something together

