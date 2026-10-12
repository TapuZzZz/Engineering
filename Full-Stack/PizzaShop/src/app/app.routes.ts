import { Routes } from '@angular/router';
import { Home } from './pages/home/home';
import { Menu } from './pages/menu/menu';
import { About } from './pages/about/about';
import { NotFound } from './pages/not-found/not-found';

export const routes: Routes = [
  { path: '', component: Home, title: 'Pizza Shop' },
  { path: 'menu', component: Menu, title: 'Menu | Pizza Shop' },
  { path: 'about', component: About, title: 'About | Pizza Shop' },
  { path: '**', component: NotFound, title: 'Page not found' }
];