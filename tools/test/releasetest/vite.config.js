import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { watch } from 'fs'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    {
      name: 'treefrog-watcher',
      configureServer(server) {
        let timer = null;
        const watcher = watch('../lib', (e, f) => {
          if (f && f.endsWith('.so') && !timer) {
            timer = setTimeout(() => {
              console.log(`${new Date().toTimeString().slice(0, 8)} hmr full reload: ${f}`);
              server.ws.send({type: 'full-reload', path: '*'});
              timer = null;
            }, 1000);  // trigger after 1000s
          }
        });
      }
    }
  ],
  build: {
    manifest: true,
    outDir: '../public',
    rollupOptions: {
      input: 'src/main.js'
    }
  }
})
