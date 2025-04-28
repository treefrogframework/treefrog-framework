import { createApp, defineAsyncComponent } from 'vue'

// Import components
const componentMap = {}
const modules = import.meta.glob('./components/*.vue')
for (const path in modules) {
  const name = path.split('/').pop().replace('.vue', '')
  componentMap[name] = defineAsyncComponent(modules[path])
}

// Execute createApp()
document.addEventListener('DOMContentLoaded', () => {
    document.querySelectorAll('[data-vue-component]').forEach(element => {
    const comp = componentMap[element.dataset.vueComponent]
    if (comp) {
      const rawProps = document.getElementById(element.dataset.vueComponent + "-props")?.textContent?.trim()
      const props = JSON.parse(rawProps || '{}')
      createApp(comp, props).mount(element)
    }
  })
})
