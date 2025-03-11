async function wait(ms) {
    return new Promise((res) => setTimeout(res, ms));
}

async function start() {
    const block = document.querySelector("#cube");
    const container = document.querySelector(".cube-container");

    let maxWidth = 0;
    let maxHeight = 0;

    if (block && container) {
        const containerBox = container.getBoundingClientRect();

        window.addEventListener("resize", () => {
            const containerBox = container.getBoundingClientRect();

            maxWidth = Math.floor(containerBox.width);
            maxHeight = Math.floor(containerBox.height);
        });

        maxWidth = Math.floor(containerBox.width);
        maxHeight = Math.floor(containerBox.height);
        let px = 0;
        let py = 0;

        while (true) {
            px++;

            if (px >= maxWidth) {
                px = 0;
                py += 20;
            }

            if (py >= maxHeight) {
                py = 0;
            }

            block.style.left = `${px}px`;
            block.style.top = `${py}px`;
            await wait(0);
        }
    }
}

start();
