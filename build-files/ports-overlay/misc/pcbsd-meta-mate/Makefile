# New ports collection makefile for:	pcbsd-meta-mate
#
# $FreeBSD$
#

PORTNAME=	pcbsd-meta-mate
PORTVERSION=	CHGVERSION
CATEGORIES=	misc
MASTER_SITES=	# empty
DISTFILES=	# none

MAINTAINER=	kris@pcbsd.org
COMMENT=	The meta-port pcbsd-meta-mate

RUN_DEPENDS=	mate-base>=0:${PORTSDIR}/x11/mate-base \
		mate-terminal>=0:${PORTSDIR}/x11/mate-terminal \
		mate-system-monitor>=0:${PORTSDIR}/sysutils/mate-system-monitor \
		eom>=0:${PORTSDIR}/graphics/mate-image-viewer \
		engrampa>=0:${PORTSDIR}/archivers/mate-file-archiver \
		atril>=0:${PORTSDIR}/graphics/mate-document-viewer \
		mate-volume-control>=0:${PORTSDIR}/audio/mate-media \
		mate-power-manager>=0:${PORTSDIR}/sysutils/mate-power-manager \
		mozo>=0:${PORTSDIR}/deskutils/mate-menu-editor \
		mate-calc>=0:${PORTSDIR}/math/mate-calc \
		mate-applets>=0:${PORTSDIR}/x11/mate-applets
NO_BUILD=	yes

do-install:
	${DO_NADA}

.include <bsd.port.mk>
