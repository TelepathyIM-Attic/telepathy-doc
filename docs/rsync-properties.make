rsync_host = www.freedesktop.org
rsync_target = $(rsync_host):/srv/telepathy.freedesktop.org/www/doc/book/

rsync_target_html = $(rsync_target)
rsync_target_pdf = $(rsync_target)/pdf/

rsync_args = -z --rsh ssh --delete-after
