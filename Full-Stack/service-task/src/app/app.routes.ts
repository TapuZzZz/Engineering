import { Routes } from '@angular/router';
import { Checkout } from '../components/checkout/checkout';
import { ShopList } from '../components/shop-list/shop-list';

export const routes: Routes = [
    { path: '', component: Checkout },
    { path: 'checkout', component: Checkout },
    { path: 'shop-list', component: ShopList },
    { path: '**', redirectTo: '' }
];
