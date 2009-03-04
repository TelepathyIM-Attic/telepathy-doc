rsync_host = dhansak
rsync_target = $(rsync_host):public_html/telepathy-book/

rsync_target_html = $(rsync_target)
rsync_target_pdf = $(rsync_target)/pdf/

rsync_args = -z --rsh ssh --delete-after
