## SO-2023/2024 Project – University of Turin

### 🛠️ Build the Docker Image

If you modify the `Dockerfile`, rebuild the image with:

```bash
docker build --platform linux/amd64 -t ubuntu-so .
```

---

### 🚀 Run the Docker Container

Replace `/absolute/path/to/your/project` with the full path to your project directory on your system.

```bash
docker run -it --name SO-Project -v "/absolute/path/to/your/project":/SO-2024 ubuntu-so
```

### 🔄 If Docker is Already Running

To check running containers:

```bash
docker ps
```

To access the container’s terminal:

```bash
docker exec -it SO-Project bash
```

> 💡 *Make sure the container name is correct when using `docker exec`.*

---
