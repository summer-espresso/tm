pkg:
	cd frontend ; npm run build
	cd backend ; dpkg-buildpackage -us -uc -b -tc
