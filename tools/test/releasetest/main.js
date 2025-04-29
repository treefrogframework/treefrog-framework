import { createApp, defineAsyncComponent } from 'vue'

// Import components
const modules = import.meta.glob('./components/*.vue')

// Execute createApp()
document.addEventListener('DOMContentLoaded', () => {
  document.querySelectorAll('[data-vue-component]').forEach(element => {
    const name = element.dataset.vueComponent
    const mod = modules[`./components/${name}.vue`]
    if (mod) {
      const rawProps = document.getElementById(name + '-props')?.textContent?.trim()
      const props = JSON.parse(rawProps || '{}')
      createApp(defineAsyncComponent(mod), props).mount(element)
    } else {
      console.error(`Component not found: ${name}.vue`)
    }
  })
})
