Git Server的搭建
===============
ba5agkai at gmail.com 2015-02-24

# Git基本概念

Git是分布式的，这句话的意思，是git没有中央存储仓库，它设计成没有。可是在大多数使用场景中，我们其实都是希望有一个中央存储仓库的，由那个仓库实现远端备份和集中交换。

Git是分布式的还体现在即使在单机上，也不存在一个git服务程序或单机自身的集中存储仓库。任何一个目录，只要执行了`git init`，就把这个目录转换成一个git管理的目录了。而实际上，git程序所做的事情，是在这个目录下建立了一个.git目录，其下还有一系列的目录和文件，那里就是存放这个目录下的所有git相关数据和版本变化数据的地方。

所以，git的做法，就是在你自己的目录下藏了一个目录，用来存放这个目录下所发生的要做版本管理的所有的事情。当我们对本地文件作了修改，要commit的时候，其实只是提交到了那个.git目录里去了。版本管理工具的一些常见操作，如checkout等，也都是在你原本的目录和这个.git目录之间发生的。

当然git本身也没有任何的权限控制，任何能访问那个.git目录的人都可以得到全部的版本历史。

git这样做的一个额外的好处，是解决了很多人先写程序后管理的毛病。svn就需要先在服务器上建立仓库，然后checkout一个空的仓库到本地，才能在本地建立起工作空间，然后才能往里面加东西。而用git的时候，可以在已经具有一些文件的工作目录上直接建立git仓库，然后在“git服务器”上建立仓库后，再进行push就可以了。

当然比svn这样的工具麻烦的地方也是在于，commit提交并没有直接把本地的修改提交到远端的服务器去，还得另外做一次push才能发送过去。

这里的git本身只是一个小程序，并没有做成OS下的一个服务，那么当然也就不存在提供网络服务了。

可是，在大多数使用场景中，我们是希望存在那么一个远端的“git服务器”的，这个时候，这个服务器又是怎么实现的呢？

git本身并不是服务，所以也就没有socket端口来接受访问连接，因此，要能让远端访问服务器上的仓库，一定得借助其他网络程序。传统上主要是借助http和ssh两种服务来做的。用http的时候，可以看作是在http服务器，如Apache上挂了一系列的CGI程序，通过CGI来实现git指令，包括数据的双向传输。而用ssh的时候，则是借助ssh本身可以登陆后直接执行远端指令后退出的方式来把文件传递到服务器后在服务器上执行git指令的。

#安装git

在Ubuntu上安装git非常简单：

	sudo apt-get install git
	
然后，可以做两个基础配置：

	$ git config --global user.name <your name>
	$ git config --global user.email <your email>
	
如果名字中间有空格，可以前后加双引号。

#配置ssh

我们打算通过ssh来访问git服务器，所以得对ssh做些配置。

##建立git用户

首先在服务器上建立一个叫做git的用户，它的家目录是将来存放仓库的地方，所以可以考虑放在/home以外的地方。另外，一开始需要它有密码，这样才能先登陆上去，以后需要把这个密码取消，这样别人就不能登陆上去了。

	sudo adduser --system --shell /bin/bash --group git

在git的家目录里创建一个.ssh目录，并且把目录的权限做成700，即只有自己可以进入。在这个目录中touch一个authorized_keys：

	touch authorized_keys

##管理机的公钥认证

在你常用的计算机上建立公钥认证，以后从这台计算机上可以ssh登陆服务器的git账号，万一有需要可以用。这台计算机被叫做管理机。

首先在管理机上执行：

	ssh-keygen -f ~/.ssh/gitserver-sh
	
这样会在家目录下的.ssh目录里制造出两个文件：

	gitserver-sh
	gitserver-sh.pub
	
其中前一个是私钥，后一个是公钥。

用scp把这个gitserver-sh.pub文件传到服务器上。然后把文件内容复制进~/.ssh/authorized_keys：

	cat gitserver-sh.pub >> ~/.ssh/authorized_keys
	
这之后，登陆服务器的git账户就不需要用户名和密码了，直接：

	ssh <gitserver_ip>

就可以了。

可是，如果服务器上还有其他账号需要ssh登陆怎么办？所以，需要修改管理机上的ssh配置：

	sudo vi /etc/ssh_config
	
下面是个例子：

	Host home
	   user someone
	   port 22
	   hostname www.someone.org
	   identityfile ~/.ssh/home

	Host gitserver-sh
	   user git
	   port 22
	   hostname www.someone.org
	   identityfile ~/.ssh/home

这里，Host后面的名字是将来用来登陆时的名字，也就是说，今后就可以这样登陆git账户：

	ssh gitserver-sh
	
ssh会根据ssh_config里的信息来决定连接哪个服务器（hostname）的哪个端口（port），然后登陆哪个账号（user），用哪个.pub文件（identityfile）。

这里我们给两个账户用了相同的.pub文件，这完全没问题。只是需要在服务器的someone和git两个用户的家目录的.ssh/authorized_keys里都放进相同的那段.pub就可以了。

如果还有其他的计算机需要登陆服务器，只要在那台计算机上照样操作，把.pub的内容加入到服务器的某个用户的.ssh/authorized_keys里就可以了。

采用了这样登陆认证方式的服务器，仍然接受没有登记.pub的机器以密码方式登陆。如果想要禁止密码方式登陆，则需要禁止那个用户的密码。在服务器上执行：

	sudo passwd -d git
	
这样，git用户就无法以密码方式登陆了。而其他用户并不受影响。

#安装gitolite

##下载gitolite

Gitolite是一个git服务管理工具，通过ssh协议公钥对用户进行认证，并能够通过配置文件对写操作进行基于分支和路径的的精细授权。Gitolite也不是服务程序，而是借助ssh来执行相应的脚本，并运用git钩子程序来完成各种功能的。它的官网是 [http://gitolite.com/gitolite/](http://gitolite.com/gitolite/)。

在服务器上以git登陆后，用git来下载：

	git clone git://github.com/sitaramc/gitolite

这样就在~git/建立了一个gitolite目录，然后，执行
	
	gitolite/install -ln
	
这样会在~git/bin下建立一个gitolite的链接，指到~git/gitolite。

##配置gitolite

然后，在管理机上制造另一个.pub：

	ssh-keygen -f ~/.ssh/myname
	
注意这个.pub必须和之前的那个gitserver-sh不同，然后在管理机的ssh_config里为它做一个别名gitserver：

	Host gitserver
		user git
		port 22
		hostname www.someone.org
		identityfile ~/.ssh/myname

把这个gitserver.pub文件传输到服务器上的~git/。在服务器上登陆git用户，在家目录下执行：

	gitolite setup -pk myname.pub

这个myname就成了在gitolite服务器里标识身份的名字，可以被看作是gitolite用户名。

这时候如果打开~git/.ssh/authorized_keys文件，会看到像这样的内容：

	# gitolite start
	command="/home/git/gitolite/src/gitolite-shell myname",no-port-forwarding,no-X11-forwarding,no-agent-forwarding,no-pty ....
	# gitolite end

今后每一个可以连上gitolite的客户机，都会在这里有这样一条记录的。

安装好gitolite之后，可以删除git用户的密码：

	sudo passwd -d git

这样别人就不可能用git用户来登陆服务器的shell了，只有管理机可以通过ssh文件认证来登陆。

##管理gitolite

Gitolite并没有实现一个常见的GUI或Web的管理界面，它的做法是在服务器上保持一个叫做gitolite-admin的仓库，那个仓库里放了一些配置数据。只要克隆了这个仓库，对其中的一些配置文件作出修改，再提交给服务器，就可以完成管理操作。用了gitolite之后，也不再需要在服务器上通过shell来执行git命令做配置了。

首先从管理机上克隆gitolite的管理库：

	git clone git@gitserver:gitolite-admin
	
这样会在管理机本地建立一个gitolite-admin目录，其中两个子目录：

* conf —— 存放配置文件
* keydir —— 存放允许访问服务器的客户机的ssh.pub文件

###conf

在conf目录中只有一个文件：gitolite.conf。目前打开可以看到这样的内容：

	repo gitolite-admin
	    RW+     =   myname

	repo testing
	    RW+     =   @all

也就是说，这是关于有什么仓库，每个仓库谁有权读写的文件。现在这个文件说，管理仓库只有myname有权读写。

###keydir

现在在keydir里面只有一个myname.pub文件。就是之前传到git用户家目录里去的那个文件，在`git setup`的时候被拷贝到这里了。

###增加新用户（机）

在要访问服务器的客户机上，用ssh-keygen产生一个.pub文件，用恰当的名字命名，然后拷贝到管理机的keydir里。

注意这个文件名只是在ssh登陆时用以分辨.pub的gitolite用户名，和git记录里的提交者名字无关。每个客户机可以设置user.name和user.email，提交的时候这个数据会保存在代码树的提交记录里。也就是说，如果两台计算机都会提交，需要做出不同的.pub文件，但是记录中仍然可能是同一个用户名字。

另外从概念上，这个.pub是对应机器的，而不是对应用户的。准确地说，是和那个机器的.ssh/的私钥文件对应的。

删除用户则只要删除它对应的.pub文件即可。

###增加新仓库

在conf/gitolite.conf里增加一条记录即可。

###提交服务器

在管理机的本地仓库中做了任何的修改，都必须提交并推到服务器后才能生效：

	git commit
	git push
	
#客户机

在客户机上用这样的URL来访问服务器：

	ssh://git@www.someone.org/reponame
	
其中www.someone.org是git服务器的名字，而reponame是仓库的名字。

如果服务器的ssh端口不是默认的22，比如是2222，那么URL是：

	ssh://git@www.someone.org:2222/reponame

当然也可以在客户机的ssh_config里配置好别名，直接用别名代替上面的网址和端口部分。

注意这里在服务器名字前面的用户名始终是git，而不是gitolite用户名。在服务器上，是根据ssh连接时提供的公钥文件来判断是哪个gitolite用户名的。
