# Lab 0: GDB&QEMU调试64位RISC-V LINUX

<center><font size=3><br/>张志心 <code>3210106357</code></font></center>



## 实验目的

+ 使用交叉编译工具，完成 Linux 内核代码编译；
+ 使用 `qemu` 运行内核；
+ 熟悉 `GDB` 和 `QEMU` 联合调试。

## 实验环境

> Mac with Apple Silicon

## 实验步骤

### 搭建实验环境

下载 Docker Desktop ，在终端输入

```bash
docker pull ubuntu:22.04
```

![image-20230924175725799](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924175725799.png)

获取 $\texttt{ubuntu22.04}$ 的镜像。然后使用

```bash
docker run --name nanlin --hostname Lisore ubuntu:22.04 /bin/bash 
```

![image-20230924175750730](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924175750730.png)

启动一个 $\texttt{ubuntu22.04}$ 的容器，并且设置容器名为 $\texttt{nanlin}$，主机名为 $\texttt{Lisore}$。

然后使用

```bash
docker start nanlin
docker attach nanlin
```

![image-20230924175910113](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924175910113.png)

启动容器。首先安装编译内核所需要的交叉编译工具链和用于构建程序的软件包

```bash
sudo apt install  gcc-riscv64-linux-gnu
sudo apt install  autoconf automake autotools-dev curl libmpc-dev libmpfr-dev libgmp-dev \
                    gawk build-essential bison flex texinfo gperf libtool patchutils bc \
                    zlib1g-dev libexpat-dev git
```

![image-20230924180049722](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924180049722.png)

> 本人已经装过相关软件包，故只作演示，下同。

安装用于启动 RISCV64 平台上的内核的模拟器 `qeme` 和调试工具 `gdb`。

```bash
sudo apt install qemu-system-misc
sudo apt install gdb-multiarch
```

![image-20230924180254256](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924180254256.png)

![image-20230924180223195](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924180223195.png)

在此之前，还需要安装 `sudo`。

```
apt-get update
apt-get install sudo
```

![image-20230924180339881](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924180339881.png)

注：若要退出容器则输入

```bash
exit
```

![image-20230924180354722](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924180354722.png)

### 获取 Linux 源码和已经编译好的文件系统

从 https://www.kernel.org 下载最新的 Linux 源码至宿主机。

![image-20230924172126717](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924172126717.png)

使用 `docker cp` 将源代码复制到容器中。

```bash
docker cp linux-6.6-rc2.tar.gz nanlin:/root/
```

![image-20230924180440469](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924180440469.png)

然后进入容器，解压该文件。

```bash
tar -zxvf linux-6.6-rc2.tar.gz
ls
```

![image-20230924180716886](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924180716886.png)

使用 `git` 克隆实验仓库，获取根文件系统的镜像。

```bash
git clone https://github.com/ZJU-SEC/os23fall-stu.git
```

![image-20230924180858423](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924180858423.png)

已经构建完成的根文件系统的镜像路径为 `os23fall-stu/src/lab0/rootf.img`。

![image-20230924180933176](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924180933176.png)

### 编译 Linux 内核

```bash
cd /root/os # linux-6.6-rc2 解压后的路径
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- defconfig    # 使用默认配置
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)   # 编译
```

![image-20230924181042895](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924181042895.png)

### 使用 QEMU 运行内核

```bash
qemu-system-riscv64 -nographic -machine virt -kernel /root/os/arch/riscv/boot/Image \
    -device virtio-blk-device,drive=hd0 -append "root=/dev/vda ro console=ttyS0" \
    -bios default -drive file=/root/os23fall-stu/src/lab0/rootfs.img,format=raw,id=hd0
```

运行后终端截图如下：

<img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924173445508.png" alt="image-20230924173445508" style="zoom: 25%;" />

>  注：退出 `QEMU` 的方法为：使用 `Ctrl+A`，**松开**后再按下 `X` 键即可退出 `QEMU`。

### 使用 GDB 对内核进行调试

开启两个终端，一个使用 `QEMU` 启动 Linux，另一个使用 `GDB` 与 `QEMU` 远程通信（使用 `tcp:1234` 端口）进行调试。

```bash
# Terminal 1
qemu-system-riscv64 -nographic -machine virt -kernel /root/os/arch/riscv/boot/Image \
    -device virtio-blk-device,drive=hd0 -append "root=/dev/vda ro console=ttyS0" \
    -bios default -drive file=/root/os23fall-stu/src/lab0/rootfs.img,format=raw,id=hd0 -S -s
# Terminal 2
docker exec -it nanlin /bin/bash # 打开第二个终端并进入容器
gdb-multiarch /root/os/vmlinux
```

![image-20230924181218051](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924181218051.png)

![image-20230924175010886](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924175010886.png)

1. 连接 `qemu`

![image-20230924174824392](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924174824392.png)

2. 设置断点

![image-20230924174941416](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924174941416.png)

3. 继续执行

![image-20230924175048185](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924175048185.png)

​	此时 `qemu` 运行到 `start_kernel()` 函数位置。

<img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924175226617.png" alt="image-20230924175226617" style="zoom: 33%;" />

4. 退出 `gdb`

![image-20230924175421316](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924175421316.png)

此时 `qemu` 运行到结束位置。

![image-20230924175530001](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924175530001.png)

## 思考题

1. 使用 `riscv64-linux-gnu-gcc` 编译单个 `.c ` 文件；

   ```bash
   vim hello.c
   ```

   ![image-20230924183325014](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924183325014.png)

   ![image-20230924183542069](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924183542069.png)

2. 使用 `riscv64-linux-gnu-objdump` 反汇编 1 中得到的编译产物；

   ![image-20230924184043552](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924184043552.png)

3. 调试 Linux 时 :

   a. 在 GDB 中查看汇编代码

   ![image-20230924182211039](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924182211039.png)

   b. 在 0x80000000 处下断点

   ![image-20230924182251471](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924182251471.png)

   c. 查看所有已下的断点

   ![image-20230924182312338](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924182312338.png)

   d. 在 0x80200000 处下断点

   ![image-20230924182404752](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924182404752.png)

   e. 清除 0x80000000 处的断点

   ![image-20230924182419786](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924182419786.png)

   f. 继续运行直到触发 0x80200000 处的断点

   ![image-20230924182535337](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924182535337.png)

   g. 单步调试一次

   ![image-20230924182749461](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924182749461.png)

   h. 退出 QEMU

   ![image-20230924182853694](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924182853694.png)

4. 使用 `make` 工具清除 Linux 的构建产物；

   ![image-20230924183000862](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab0/assets/image-20230924183000862.png)

5. `vmlinux` 和 `Image` 的关系和区别是什么？

   `vmlinux` 是链接后生成的 `linux elf` 文件，不是镜像格式，`Image` 是对 `vmlinux` 的 `elf` 格式进行解析后生成的镜像文件，可直接引导系统启动。

